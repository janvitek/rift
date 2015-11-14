#ifndef GC_H
#define GC_H

#include <array>
#include <deque>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <iostream>
#include <cassert>
#include <cstring>

// #define GC_DEBUG 1

namespace gc {

typedef size_t index_t;

// NodeIndex is a container to pass around the location of an object on the 
// heap. Location is given by page index and object index (relative to page).
struct NodeIndex {
public:
    static constexpr index_t MaxPage = 0xffffffff;

    NodeIndex(index_t pageNum, index_t idx) : pageNum(pageNum), idx(idx) {}
    const index_t pageNum;
    const index_t idx;

    static const NodeIndex & nodeNotFound() {
        static NodeIndex nodeNotFound(MaxPage, 0);
        return nodeNotFound;
    }

    bool found() const {
        return pageNum != MaxPage;
    }
};


// A Page of memory holding a constant number of equally sized objects T
template <typename T>
class Page {
public:
    // Node is the memory allocated to hold one T
    struct Node {
        uint8_t data[sizeof(T)];
    };
    static_assert(sizeof(Node) == sizeof(T), "");

    static constexpr size_t nodeSize = sizeof(Node);
    static constexpr index_t pageSize = 3600 / nodeSize;
    static constexpr size_t size = pageSize * nodeSize;

    // A valid object is:
    // 1. Within the page boundaries
    // 2. Pointing to the beginning of a Node cell
    // 3. Pointing to a node cell which is not unused
    bool isValidObj(void* ptr) const {
        auto addr = reinterpret_cast<uintptr_t>(ptr);

        if (addr < first || addr > last)
            return false;

        auto idx = getClosestIndex(addr);

        return reinterpret_cast<uintptr_t>(&node[idx]) == addr &&
        isAlloc[idx];
    }

    index_t getIndex(void* ptr) const {
        auto addr = reinterpret_cast<uintptr_t>(ptr);
        return getClosestIndex(addr);
    }

    T* alloc() {
        if (freelist.size()) {
            auto idx = freelist.front();
            freelist.pop_front();

            assert(!isAlloc[idx] && !mark[idx]);
            isAlloc[idx] = true;

            return getAt(idx);
        }
        return nullptr;
    }

    Page() : first(reinterpret_cast<uintptr_t>(&node[0])),
    last(reinterpret_cast<uintptr_t>(&node[pageSize - 1])) {
        assert(getClosestIndex((uintptr_t)first + 1) == 0);
        assert(getClosestIndex((uintptr_t)last + 1) == pageSize - 1);

        mark.fill(false);
        isAlloc.fill(false);
        for (index_t i = 0; i < pageSize; ++i)
            freelist.push_back(i);
    }

    void sweep() {
        for (index_t i = 0; i < pageSize; ++i) {
            if (!mark[i]) {
                if (isAlloc[i]) {
                    freeNode(i);
                }
            } else {
                mark[i] = false;
            }
        }
    }

    index_t free() const {
        return freelist.size() * nodeSize;
    }

    void verify() const {
        for (auto i : freelist)
            assert(!mark[i] && !isAlloc[i]);

        for (index_t i = 0; i < pageSize; ++i) {
            assert(isAlloc[i] || !mark[i]);
        }
    }

    bool empty() const {
        return freelist.size() == pageSize;
    }

    void setMark(index_t idx) {
        assert(isAlloc[idx]);
        assert(!mark[idx]);
        mark[idx] = true;
    }

    bool isMarked(index_t idx) const {
        return mark[idx];
    }

    Page(Page const &) = delete;
    void operator= (Page const &) = delete;

    const uintptr_t first, last;

private:
    // Freeing a node will
    // 1. call the destructor of said object
    // 2. push the cell to the freelist
    void freeNode(index_t idx) {
        getAt(idx)->~T();
#ifdef GC_DEBUG
        memset(&node[idx], 0xd, nodeSize);
#endif
        freelist.push_back(idx);
        isAlloc[idx] = false;
    }

    T* getAt(index_t idx) {
        return reinterpret_cast<T*>(&node[idx]);
    }

    index_t getClosestIndex(uintptr_t addr) const {
        uintptr_t start = reinterpret_cast<uintptr_t>(&node);
        // Integer division -> floor
        return (addr - start) / nodeSize;
    }

    std::array<Node, pageSize> node;
    std::array<bool, pageSize> mark;
    std::array<bool, pageSize> isAlloc;
    std::deque<index_t> freelist;
};

// Arena implements the arena interface plus the alloc()
// function. Memory is provided by a pool of Page<T>'s.
template <typename T>
class Arena {
public:
    static constexpr size_t nodeSize = Page<T>::nodeSize;

    T* alloc(bool grow) {
        for (auto p : page) {
            auto res = p->alloc();
            if (res) return res;
        }
        if (page.size() > 0 && !grow) return nullptr;
        if (page.size() == NodeIndex::MaxPage) {
            throw std::bad_alloc();
        }
        auto p = new Page<T>;
        registerPage(p);
        auto n = p->alloc();
        assert(n);
        return n;
    }

    NodeIndex findNode(void* ptr) {
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);

        // Simple performance hack, which allows us to quickly discard
        // implausible pointers without iterating the page list.
        if (addr < minAddr || addr > maxAddr)
            return NodeIndex::nodeNotFound();

        for (index_t i = 0; i < page.size(); i++) {
            if (page[i]->isValidObj(ptr)) {
                return NodeIndex(i, page[i]->getIndex(ptr));
            }
        }

        return NodeIndex::nodeNotFound();
    }

    void sweep() {
        for (auto pi = page.begin(); pi != page.end(); ) {
            auto p = *pi;
            p->sweep();
            // If a page is completely empty we release it
            if (p->empty()) {
                delete p;
                pi = page.erase(pi);
            } else {
                pi++;
            }
        }
    }

    size_t free() const {
        size_t f = 0;
        for (auto p : page)
            f += p->free();
        return f;
    }

    size_t size() const {
        return page.size() * Page<T>::size;
    }

    void verify() const {
        for (auto p : page)
            p->verify();
    }

    bool isMarked(NodeIndex & i) const {
        return page[i.pageNum]->isMarked(i.idx);
    }

    void setMark(NodeIndex & i) {
        page[i.pageNum]->setMark(i.idx);
    }

    Arena() : minAddr(-1), maxAddr(0) {}

    ~Arena() {
        for (auto p : page) {
            delete p;
        }
        page.clear();
    }

    Arena(Arena const &) = delete;
    void operator= (Arena const &) = delete;

private:
    void registerPage(Page<T> * p) {
        // TODO: we never update the boundaries if we remove a page
        page.push_front(p);
        if (p->first < minAddr)
            minAddr = p->first;
        if (p->last > maxAddr)
            maxAddr = p->last;
    }

    std::deque<Page<T> *> page;
    uintptr_t minAddr, maxAddr;
};


class GarbageCollector {
public:
    constexpr static size_t INITIAL_HEAP_SIZE = 4096;
    constexpr static double GC_GROW_RATE = 1.3f;

    // Interface to request memory from the GC
    template <typename T>
    static void* alloc() {
        return inst().doAlloc<T>();
    }

    // GC users must implement the recursion for their Object types.
    // They can leave the implementation empty for leaf nodes or delegate to
    // genericMarkImpl for a naive but simple heap scan.
    template <typename T>
    void markImpl(T * obj);

    // A very generic mark implementation who visits every possible slot of
    // an object.
    template <typename T>
    void genericMarkImpl(T * obj) {
        static_assert(sizeof(T) % sizeof(void*) == 0,
                "Cannot deal with unaligned objects");

        void ** finger = reinterpret_cast<void**>(obj);
        size_t slots = sizeof(obj) / sizeof(void*);

        for (size_t i = 0; i < slots; ++i) {
            markMaybe(*finger);
            ++finger;
        }
    }

private:
    template <typename T>
    void* doAlloc() {
        auto res = arena<T>().alloc(false);
        if (!res) {
            doGc();
            res = arena<T>().alloc(false);
            if (!res) res = arena<T>().alloc(true);
        }

        assert(maybePointer(res));

        return res;
    }

    // The following template foo makes sure we have one arena for each
    // HeapObject type. The area is available through area<T>() function.

    template <typename T>
    struct ArenaInst {
    public:
        static Arena<T> & get() {
            static Arena<T> arena;
            return arena;
        }
    };

    template <typename T>
    Arena<T> & arena() const {
        return ArenaInst<T>::get();
    }

    size_t size() const;

    size_t free() const;

    void scanStack() {
        // Clobber all registers:
        // -> forces all variables currently hold in registers to be spilled
        //    to the stack where our stackScan can find them.
        __asm__ __volatile__ ( "nop" : :
                : "%rax", "%rbx", "%rcx", "%rdx", "%rsi", "%rdi",
                "%r8", "%r9", "%r10", "%r11", "%r12",
                "%r13", "%r14", "%r15");
        _scanStack();
    }

    // Performance Hack: we assume the first page of memory not to contain any
    // heap pointers. Maybe there is also a reasonable upper limit?
    bool maybePointer(void* p) {
        return p > (void*)4096;
    }

    const void * BOTTOM_OF_STACK;

    // The stack scan traverses the memory of the C stack and looks at every
    // possible stack slot. If we find a valid heap pointer we mark the
    // object as well as all objects reachable through it as live.
    void __attribute__ ((noinline)) _scanStack() {
        void ** p = (void**)__builtin_frame_address(0);
        while (p < BOTTOM_OF_STACK) {
            if (maybePointer(*p))
                markMaybe(*p);
            p++;
        }
    }

    void mark() {
        // TODO: maybe some mechanism to define static roots?
        scanStack();
    }

    // The core mark & sweep algorithm
    void doGc() {
#ifdef GC_DEBUG
        unsigned memUsage = size() - free();
        verify();
#endif

        mark();

#ifdef GC_DEBUG
        verify();
#endif

        sweep();

#ifdef GC_DEBUG
        verify();
        unsigned memUsage2 = size() - free();
        assert(memUsage2 <= memUsage);
        std::cout << "reclaiming " << memUsage - memUsage2
        << "b, used " << memUsage2 << "b, total " << size() << "b\n";
#endif
    }

    // First step of marking is to determine if a pointer is a valid object and
    // if yes to find the arena and page which contains it. Then the type
    // specific part is handled in mark<T>(ptr, i).
    void markMaybe(void * ptr);

    // Sometimes we already know the type of a pointer which gives us faster
    // lookup.
    template <typename T>
    void mark(T * obj) {
        auto i = arena<T>().findNode(obj);
        assert(i.found());
        mark(obj, i);
    }

    // Generic mark algorithm is to check if an object was marked before. Iff
    // not we will mark it first (to deal with cycles) and then delegate to the
    // user defined markImpl<T> for the specific type to scan its child nodes.
    template <typename T>
    void mark(T * obj, NodeIndex & i) {
        auto & a = arena<T>();
        if (!a.isMarked(i)) {
            a.setMark(i);
            markImpl<T>(obj);
        }
    }

    // Delete everything which is not reachable anymore
    void sweep();

    void verify() const;

    static GarbageCollector & inst() {
        static GarbageCollector gc;
        return gc;
    }

    // TODO: setting the BOTTOM_OF_STACK here is a bit brittle, as we just
    // assume that the site of the first allocation is the earliest call frame
    // holding live objects.
    GarbageCollector() : BOTTOM_OF_STACK(__builtin_frame_address(0)) { }

    GarbageCollector(GarbageCollector const &) = delete;
    void operator=(GarbageCollector const &) = delete;
};


} // namespace gc


template <typename OBJECT>
struct HeapObject {
    // Overriding new makes sure memory is allocated through the GC
    static void* operator new(size_t sz) {
        return gc::GarbageCollector::alloc<OBJECT>();
    }

    void operator delete(void*) = delete;
};

#endif

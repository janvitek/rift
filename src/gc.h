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

#ifndef __GNUG__
#include <intrin.h>
#endif

// #define GC_DEBUG 1

class RVal;
void gc_scanStack();

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

class Page {
public:
    static constexpr size_t nodeSize = 64;

    struct Node {
        uint8_t data[nodeSize];
    };
    struct Free {
        uint8_t nodes;
        Free* next;
    };
    static_assert(sizeof(Node) == nodeSize, "");
    static_assert(sizeof(Node) >= sizeof(Free), "");

    static constexpr index_t pageSize = 3600 / nodeSize;
    static constexpr size_t size = pageSize * nodeSize;

    static_assert(pageSize <= 256, "");

    size_t size2nodes(size_t size) {
        if (size % nodeSize == 0)
            return size / nodeSize;
        return 1 + (size / nodeSize);
    }

    // A valid object is:
    // 1. Within the page boundaries
    // 2. Pointing to the beginning of a Node cell
    // 3. Pointing to a node cell which is not unused
    bool isValidObj(void* ptr) const {
        auto addr = reinterpret_cast<uintptr_t>(ptr);

        if (addr < first || addr > last)
            return false;

        auto idx = getClosestIndex(addr);

        return reinterpret_cast<uintptr_t>(&node[idx]) == addr && objSize[idx];
    }

    index_t getIndex(void* ptr) const {
        auto addr = reinterpret_cast<uintptr_t>(ptr);
        return getClosestIndex(addr);
    }

    void* alloc(size_t size) {
        size_t needed = size2nodes(size);

        Free* prev = &freelist;
        Free* cur = freelist.next;

        while (cur) {
            if (cur->nodes >= needed) {
                auto idx = getIndex(cur);

                if (cur->nodes > needed) {
                    Free* next = (Free*)getAt(idx+needed);
                    next->nodes = cur->nodes - needed;
                    next->next = cur->next;
                    prev->next = next;
                    freeSpace -= nodeSize * needed;
                } else {
                    prev->next = cur->next;
                    freeSpace -= nodeSize * cur->nodes;
                }

                assert(!objSize[idx] && !mark[idx]);
                objSize[idx] = needed;

                return cur;
            }
            prev = cur;
            cur = cur->next;
        }
        return nullptr;
    }

    Page() : first(reinterpret_cast<uintptr_t>(&node[0])),
    last(reinterpret_cast<uintptr_t>(&node[pageSize - 1])) {
        assert(getClosestIndex((uintptr_t)first + 1) == 0);
        assert(getClosestIndex((uintptr_t)last + 1) == pageSize - 1);

        mark.fill(false);
        objSize.fill(0);

        Free* first = (Free*)getAt(0);
        first->next = nullptr;
        first->nodes = pageSize;
        freelist.next = first;
        freelist.nodes = 0;

        freeSpace = pageSize * nodeSize;
    }

    void sweep() {
        for (index_t i = 0; i < pageSize; ++i) {
            if (!mark[i]) {
                if (objSize[i]) {
                    freeNode(i);
                }
            } else {
                mark[i] = false;
            }
        }
    }

    index_t free() const {
        return freeSpace;
    }

    void verify() const {
        size_t foundFree = 0;
        Free* f = freelist.next;
        while (f) {
            auto i __attribute__((unused)) = getIndex(f);
            assert(!mark[i] && !objSize[i]);
            foundFree += f->nodes * nodeSize;
            f = f->next;
        }
        assert(foundFree == freeSpace);

        for (index_t i = 0; i < pageSize; ++i) {
            assert(objSize[i] || !mark[i]);
        }
    }

    bool empty() const {
        return freeSpace == pageSize;
    }

    void setMark(index_t idx) {
        assert(idx < pageSize);
        assert(objSize[idx]);
        assert(!mark[idx]);
        mark[idx] = true;
    }

    bool isMarked(index_t idx) const {
        assert(idx < pageSize);
        return mark[idx];
    }

    Page(Page const &) = delete;
    void operator= (Page const &) = delete;

    const uintptr_t first, last;

private:
    // Freeing an object
    void freeNode(index_t idx) {
        // TODO destructor??
 
#ifdef GC_DEBUG
        memset(&node[idx], 0xd, nodeSize);
#endif

        freeSpace += objSize[idx] * nodeSize;

        Free* f = (Free*)getAt(idx);
        f->next = freelist.next;
        f->nodes = objSize[idx];
        freelist.next = f;
        objSize[idx] = 0;
    }

    void* getAt(index_t idx) {
        return &node[idx];
    }

    index_t getClosestIndex(uintptr_t addr) const {
        uintptr_t start = reinterpret_cast<uintptr_t>(&node);
        // Integer division -> floor
        return (addr - start) / nodeSize;
    }

    std::array<Node, pageSize> node;
    std::array<bool, pageSize> mark;
    std::array<uint8_t, pageSize> objSize;
    Free freelist;
    size_t freeSpace;
};

class Arena {
public:
    void* alloc(size_t sz, bool grow) {
        for (auto p : page) {
            auto res = p->alloc(sz);
            if (res) return res;
        }
        if (page.size() > 0 && !grow) return nullptr;
        if (page.size() == NodeIndex::MaxPage) {
            throw std::bad_alloc();
        }
        auto p = new Page;
        registerPage(p);
        auto n = p->alloc(sz);
        assert(n);
        return n;
    }

    NodeIndex findNode(void* ptr) {
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);

        // quickly discard implausible pointers
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
        return page.size() * Page::size;
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

    void mark(void* ptr) {
        for (index_t i = 0; i < page.size(); i++) {
            if (page[i]->isValidObj(ptr)) {
                page[i]->setMark(page[i]->getIndex(ptr));
                return;
            }
        }
        assert(false);
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
    void registerPage(Page * p) {
        // TODO: we never update the boundaries if we remove a page
        page.push_front(p);
        if (p->first < minAddr)
            minAddr = p->first;
        if (p->last > maxAddr)
            maxAddr = p->last;
    }

    std::deque<Page*> page;
    uintptr_t minAddr, maxAddr;
};


class GarbageCollector {
public:
    constexpr static size_t INITIAL_HEAP_SIZE = 4096;
    constexpr static double GC_GROW_RATE = 1.3f;

    // Interface to request memory from the GC
    static void* alloc(size_t sz) {
        return inst().doAlloc(sz);
    }

private:
    Arena arena;

    void* doAlloc(size_t sz) {
        void* res = arena.alloc(sz, false);
        if (!res) {
            doGc();
            res = arena.alloc(sz, false);
            if (!res) res = arena.alloc(sz, true);
        }
        assert(res);
        return res;
    };

    size_t size() const {
        return arena.size();
    }

    size_t free() const {
        return arena.free();
    }

    void scanStack() {
        // Clobber all registers:
        // -> forces all variables currently hold in registers to be spilled
        //    to the stack where our stackScan can find them.
        __asm__ __volatile__("nop" : :
            : "%rax", "%rbx", "%rcx", "%rdx", "%rsi", "%rdi",
            "%r8", "%r9", "%r10", "%r11", "%r12",
            "%r13", "%r14", "%r15");
        gc_scanStack();
    }

    const void * BOTTOM_OF_STACK;

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

    void mark(NodeIndex& i, RVal* val) {
        assert(i.found());
        if (!arena.isMarked(i)) {
            arena.setMark(i);
            visitChildren(val);
        }
    }

    void markMaybe(void * ptr) {
        auto i = arena.findNode(ptr);
        if (i.found())
            mark(i, (RVal*)ptr);
    }

    void mark(RVal* val) {
        auto i = arena.findNode(val);
        mark(i, val);
    }

    void visitChildren(RVal*);

    // Delete everything which is not reachable anymore
    void sweep() {
        arena.sweep();
    }

    void verify() const {
        arena.verify();
    }

    static GarbageCollector & inst() {
        static GarbageCollector gc;
        return gc;
    }

    // TODO: setting the BOTTOM_OF_STACK here is a bit brittle, as we just
    // assume that the site of the first allocation is the earliest call frame
    // holding live objects.
#ifdef __GNUG__
    GarbageCollector() : BOTTOM_OF_STACK(__builtin_frame_address(0)) { }
#else
    GarbageCollector() : BOTTOM_OF_STACK(_AddressOfReturnAddress()) {}
#endif

    GarbageCollector(GarbageCollector const &) = delete;
    void operator=(GarbageCollector const &) = delete;

    friend void ::gc_scanStack();
};

} // namespace gc


#endif

#pragma once

#include <array>
#include <deque>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <iostream>
#include <cassert>
#include <cstring>
#include <memory>

#ifndef __GNUG__
#include <intrin.h>
#endif

#include "rval.h"

// #define GC_DEBUG 1

extern "C" void scanStack_();

namespace gc {

#ifdef GC_DEBUG
constexpr static Mark MARKED = 3;
constexpr static Mark UNMARKED = 7;
#else
constexpr static Mark MARKED = 1;
constexpr static Mark UNMARKED = 0;
#endif

typedef uint8_t BlockIdx;

class Page {
public:
    static constexpr size_t blockSize = 32;
    static constexpr size_t blockBits = 5;
    static_assert(1 << blockBits == blockSize, "");

    static constexpr uintptr_t pointerMask = blockSize-1;
    struct Block { uint8_t data[blockSize]; };

    static_assert(sizeof(Block) == blockSize, "");
    static_assert(sizeof(Block) > sizeof(RVal), "");

    // The number of blocks per page must be adjusted such that the resulting
    // sizeof(Page) makes good use of a physicl memory page (usually 4k)
    static constexpr BlockIdx pageSize = 120;
    static constexpr size_t size = pageSize * blockSize;

    static_assert(pageSize < (1 << 8*sizeof(BlockIdx)), "");

    // The memory for objects is partitinoned in addressable blocks
    std::array<Block, pageSize> block;
    // The index of the first block of an object is used to store the obj size
    // in blocks. All subsequent objSize[i] blocks belong to the object at i.
    std::array<BlockIdx, pageSize> objSize;

    // Freelist entry.
    // When a block is unused (ie. objSize[i] == 0) then we store a Free struct
    // in the block and add it to the freelist linked list.
    struct Free {
        BlockIdx blocks;
        Free* next;
    };
    static_assert(sizeof(Block) >= sizeof(Free), "");

    // Head of the freelist. The actual entries start at freelist.next and are
    // placed directly in the pages (using one block to holt one Free
    // struct).
    Free freelist;

    // Total free space in blocks
    size_t freeSpace;

    BlockIdx size2blocks(size_t size) {
        if (size % blockSize == 0)
            return size / blockSize;
        return 1 + (size / blockSize);
    }

    // This function must be called with a plausible pointer
    // (ie. block aligned)!
    // A valid object is:
    // 1. Within the page boundaries
    // 2. Pointing to a block in use
    inline bool isValidObj(void* ptr) const {
        auto addr = reinterpret_cast<uintptr_t>(ptr);
        assert((addr & Page::pointerMask) == 0);

        if (addr < first || addr > last)
            return false;

        auto idx = getIndex(ptr);
        return objSize[idx];
    }

    BlockIdx getIndex(void* ptr) const {
        uintptr_t p = reinterpret_cast<uintptr_t>(ptr);
        size_t idx = (p-first) >> blockBits;
#ifdef GC_DEBUG
        assert(idx < pageSize); 
#endif
        return idx;
    }

    inline RVal* free2rval(Free* cur, Free* prev, BlockIdx sz) {
        auto idx = getIndex(cur);
        if (cur->blocks > sz) {
            // In case not all blocks are needed we create a new
            // freelist entry with the remaining blocks.
            Free* next = reinterpret_cast<Free*>(getAt(idx+sz));
            next->blocks = cur->blocks - sz;
            next->next = cur->next;
            prev->next = next;
            freeSpace -= sz;
        } else {
            prev->next = cur->next;
            freeSpace -= cur->blocks;
        }

        assert(!objSize[idx]);
        objSize[idx] = sz;

        RVal* obj = getAt(idx);
        obj->mark = UNMARKED;

        return obj;
    }

    inline RVal* alloc(size_t size) {
        if (!freelist.next)
            return nullptr;

        size_t needed = size2blocks(size);

        Free* prev = &freelist;
        Free* cur = freelist.next;

        // Search for the first node on the freelist which has enough blocks
        // available.
        while (cur) {
            if (cur->blocks >= needed)
                return free2rval(cur, prev, needed);
            prev = cur;
            cur = cur->next;
        }
        return nullptr;
    }

    Page(uint8_t* store)
          : store(store),
            first(reinterpret_cast<uintptr_t>(&block[0])),
            last(reinterpret_cast<uintptr_t>(&block[pageSize - 1])) {

        // Some sanity checks. Page must be aligned
        assert(((uintptr_t)&block[3] & pointerMask) == 0);
        assert(getIndex(&block[3]) == 3);

        objSize.fill(0);
        memset(&block[0], 0, size);

        Free* first = (Free*)getAt(0);
        first->next = nullptr;
        first->blocks = pageSize;
        freelist.next = first;

        freeSpace = pageSize;
    }

    void sweep() {
        BlockIdx i = 0;
        while (i < pageSize) {
            auto sz = objSize[i];
            if (sz == 0) {
                ++i;
                continue;
            }

            if (getAt(i)->mark == UNMARKED)
                freeBlock(i);

            getAt(i)->mark = UNMARKED;
            i += sz;
        }
    }

    size_t free() const {
        return freeSpace * blockSize;
    }

    void verify();

    bool empty() const {
        return freeSpace == pageSize;
    }

    Page(Page const &) = delete;
    void operator= (Page const &) = delete;

    // Address of the memory base of this page. Use this to free the page!
    uint8_t* store;

    // Address of the first and last block.
    const uintptr_t first, last;

private:
    // Freeing an object
    void freeBlock(BlockIdx idx) {
        // TODO destructor??

        BlockIdx sz = objSize[idx];

#ifdef GC_DEBUG
        RVal* old = getAt(idx);
        assert(old->mark == UNMARKED);
        memset(old, 0xd, sz * blockSize);
#endif
        freeSpace += sz;

        Free* f = reinterpret_cast<Free*>(getAt(idx));
        f->next = freelist.next;
        f->blocks = sz;
        freelist.next = f;

        objSize[idx] = 0;
    }

    RVal* getAt(BlockIdx idx) {
        return reinterpret_cast<RVal*>(&block[idx]);
    }
};


class Arena {
public:
    static constexpr size_t pageBufferSize = sizeof(Page) +
                                             2 * sizeof(Page::blockSize);

    // The target is to allocate a Page which spans one physical pages
    // Given the block size of 32 this will result in a bit less than 250
    // blocks per Page which makes good use of the BlockSize (uint8_t) objSize
    // map.
    static_assert(pageBufferSize <= 4096, "");
    static_assert(pageBufferSize > 0.98*4096, "");

    // Allocate an RVal of size sz in bytes. If grow is true we are allowed to
    // grow the arena (ie. allocate a new page).
    RVal* alloc(size_t sz, bool grow) {
        for (auto p : pageList) {
            if (p->free() < sz)
                continue;
            auto res = p->alloc(sz);
            if (res)
                return res;
        }

        if (pageList.size() > 0 && !grow)
            return nullptr;

        // Allocate a new page. Page must be block aligned.
        uint8_t* store = new uint8_t[pageBufferSize];
        uintptr_t pn = reinterpret_cast<uintptr_t>(store);
        uintptr_t aligned = (pn + Page::blockSize - 1) & - Page::blockSize;
        void* place = reinterpret_cast<void*>(aligned);
        assert(place >= store &&
                reinterpret_cast<uintptr_t>(store) + pageBufferSize >= 
                reinterpret_cast<uintptr_t>(place) + sizeof(Page));
        auto p = new (place) Page(store);
        registerPage(p);

        auto n = p->alloc(sz);
        if (!n) throw std::bad_alloc();
        return n;
    }

    inline bool isValidObj(void* ptr) const {
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);

        // quickly discard implausible pointers
        if ((addr & Page::pointerMask) != 0)
            return false;

        assert(minAddr < maxAddr);

        // discard pointers outside allocated space
        if (addr < minAddr || addr > maxAddr)
            return false;

        for (auto p : pageList) {
            if (p->isValidObj(ptr)) {
                return true;
            }
        }

        return false;
    }

    void sweep() {
        for (auto pi = pageList.begin(); pi != pageList.end(); ) {
            auto p = *pi;
            p->sweep();
            // If a page is completely empty we release it
            if (p->empty()) {
#ifdef GC_DEBUG
                std::cout << "Released a Page\n";
#endif
                delete [] p->store;
                pi = pageList.erase(pi);
            } else {
                pi++;
            }
        }
    }

    size_t free() const {
        size_t f = 0;
        for (auto p : pageList)
            f += p->free();
        return f;
    }

    size_t size() const {
        return pageList.size() * Page::size;
    }

    void verify() const {
        for (auto p : pageList)
            p->verify();
    }

    Arena() : minAddr(-1), maxAddr(0) {}

    ~Arena() {
        for (auto p : pageList) {
            delete [] p->store;
        }
        pageList.clear();
    }

    Arena(Arena const &) = delete;
    void operator= (Arena const &) = delete;

    std::deque<Page*> pageList;

private:
    void registerPage(Page * p) {
#ifdef GC_DEBUG
      std::cout << "Allocated a new Page\n";
#endif
        // TODO: we never update the boundaries if we remove a page
        pageList.push_front(p);
        if (p->first < minAddr)
            minAddr = p->first;
        if (p->last > maxAddr)
            maxAddr = p->last;
    }

    uintptr_t minAddr, maxAddr;
};


class GarbageCollector {
public:
    // Interface to request memory from the GC
    static RVal* alloc(size_t sz, Type type) {
        return inst().doAlloc(sz, type);
    }

private:
    // Currently we have just one arena. A possible optimization would be to
    // have different arenas for different size buckets.
    Arena arena;

    constexpr static size_t INITIAL_HEAP_SIZE = 4*Page::size;
    constexpr static size_t MIN_HEAP_SIZE = 4*Page::size;
    static_assert (INITIAL_HEAP_SIZE >= MIN_HEAP_SIZE, "");

    size_t heapLimit = INITIAL_HEAP_SIZE;
    constexpr static double HEAP_MIN_FREE = 0.1f;
    constexpr static double HEAP_MAX_FREE = 0.4f;
    constexpr static double HEAP_GROW_RATIO = 1.2f;
    constexpr static double HEAP_SHRINK_RATIO = 0.8f;

    RVal* doAlloc(size_t sz, Type type) {
        RVal* res = arena.alloc(sz, arena.size() < heapLimit);

        //  Allocation failed
        if (!res) {
            doGc();

            if ((float)free() / (float)size() < HEAP_MIN_FREE) {
                heapLimit *= HEAP_GROW_RATIO;
            } else if ((float)free() / (float)size() > HEAP_MAX_FREE &&
                    heapLimit > MIN_HEAP_SIZE) {
                heapLimit *= HEAP_SHRINK_RATIO;
                heapLimit =
                    heapLimit < MIN_HEAP_SIZE ? MIN_HEAP_SIZE : heapLimit;
            }

            res = arena.alloc(sz, true);
        }

        // TODO: add a backup allocator for huge objects. Currently allocations
        // bigger than Page::size will fail.
        if (!res) throw std::bad_alloc();

        res->type = type;
        return res;
    };

    size_t size() const {
        return arena.size();
    }

    size_t free() const {
        return arena.free();
    }

    const void * BOTTOM_OF_STACK;

    void scanStack();
    void mark();

    void doGc();

    inline bool isValidObj(void* ptr) const {
        return arena.isValidObj(ptr);
    }

    void mark(RVal* val) {
#ifdef GC_DEBUG 
        assert(isValidObj(val));
#endif
        if (val->mark == MARKED)
            return;
        assert(val->mark == UNMARKED);
        val->mark = MARKED;
        visitChildren(val);
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
    friend void ::scanStack_();
};

} // namespace gc


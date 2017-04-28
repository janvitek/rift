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

#include "rval.h"

// #define GC_DEBUG 1

namespace gc {

typedef size_t index_t;

class Page {
public:
    static constexpr size_t blockSize = 32;

    struct Block {
        uint8_t data[blockSize];
    };
    struct Free {
#ifdef GC_DEBUG
        void* canary[2];
        static constexpr void* CANARY = (void*)0xafafafaf;
#endif
        void tag() {
#ifdef GC_DEBUG
          canary[0] = canary[1] = CANARY;
#endif
        }
        void checkTag() {
#ifdef GC_DEBUG
          assert(canary[0] == CANARY);
          assert(canary[1] == CANARY);
#endif
        }
        uint8_t blocks;
        Free* next;
    };
    static_assert(sizeof(Block) == blockSize, "");
    static_assert(sizeof(Block) >= sizeof(Free), "");
    static_assert(sizeof(Block) >= sizeof(RVal), "");

    static constexpr index_t pageSize = 3600 / blockSize;
    static constexpr size_t size = pageSize * blockSize;

    static_assert(pageSize <= 256, "");

    size_t size2blocks(size_t size) {
        if (size % blockSize == 0)
            return size / blockSize;
        return 1 + (size / blockSize);
    }

    // A valid object is:
    // 1. Within the page boundaries
    // 2. Pointing to the beginning of a block
    // 3. Pointing to a block which is in use
    bool isValidObj(void* ptr) const {
        auto addr = reinterpret_cast<uintptr_t>(ptr);

        if (addr < first || addr > last)
            return false;

        auto idx = getClosestIndex(addr);

        return reinterpret_cast<uintptr_t>(&block[idx]) == addr && objSize[idx];
    }

    index_t getIndex(void* ptr) const {
        auto addr = reinterpret_cast<uintptr_t>(ptr);
        return getClosestIndex(addr);
    }

    RVal* alloc(size_t size) {
        size_t needed = size2blocks(size);

        Free* prev = &freelist;
        Free* cur = freelist.next;

        while (cur) {
            if (cur->blocks >= needed) {
                auto idx = getIndex(cur);

                cur->checkTag();
                if (cur->blocks > needed) {
                    Free* next = (Free*)getAt(idx+needed);
                    next->blocks = cur->blocks - needed;
                    next->next = cur->next;
                    next->tag();
                    prev->next = next;
                    freeSpace -= blockSize * needed;
                } else {
                    prev->next = cur->next;
                    freeSpace -= blockSize * cur->blocks;
                }

                assert(!objSize[idx]);
                objSize[idx] = needed;

                RVal* obj = getAt(idx);
                obj->mark = 0;

                return obj;
            }
            prev = cur;
            cur = cur->next;
        }
        return nullptr;
    }

    Page() : first(reinterpret_cast<uintptr_t>(&block[0])),
             last(reinterpret_cast<uintptr_t>(&block[pageSize - 1])) {
        assert(getClosestIndex((uintptr_t)first + 1) == 0);
        assert(getClosestIndex((uintptr_t)last + 1) == pageSize - 1);

        objSize.fill(0);
        memset(&block[0], 0, pageSize*blockSize);

        Free* first = (Free*)getAt(0);
        first->next = nullptr;
        first->blocks = pageSize;
        freelist.next = first;
        freelist.blocks = 0;
        first->tag();

        freeSpace = pageSize * blockSize;
    }

    void sweep() {
        index_t i = 0;
        while (i < pageSize) {
            auto sz = objSize[i];
            if (sz) {
                if (!getAt(i)->mark) {
                    freeBlock(i);
                } else {
                    getAt(i)->mark = 0;
                }
                i += sz;
            } else {
                ++i;
            }
        }
    }

    index_t free() const {
        return freeSpace;
    }

    void verify();

    bool empty() const {
        return freeSpace == pageSize;
    }

    Page(Page const &) = delete;
    void operator= (Page const &) = delete;

    const uintptr_t first, last;

private:
    // Freeing an object
    void freeBlock(index_t idx) {
        // TODO destructor??

        size_t freed = objSize[idx] * blockSize;

#ifdef GC_DEBUG
        RVal* old = getAt(idx);
        assert(old->mark == 0);
        memset(old, 0xd, freed);
#endif
        freeSpace += freed;

        Free* f = (Free*)getAt(idx);
        f->next = freelist.next;
        f->blocks = objSize[idx];
        freelist.next = f;
        objSize[idx] = 0;

        f->tag();
    }

    RVal* getAt(index_t idx) {
        return reinterpret_cast<RVal*>(&block[idx]);
    }

    index_t getClosestIndex(uintptr_t addr) const {
        uintptr_t start = reinterpret_cast<uintptr_t>(&block);
        // Integer division -> floor
        return (addr - start) / blockSize;
    }

    // The memory for objects is partitinoned in addressable blocks
    std::array<Block, pageSize> block;
    // The index of the first block of an object is used to store the obj size
    // in blocks
    std::array<uint8_t, pageSize> objSize;
    // The freelist is internal. We just cast &block[] to Free* to create a
    // linked list of free blocks.
    Free freelist;
    size_t freeSpace;
};


class Arena {
public:
    RVal* alloc(size_t sz, bool grow) {
        for (auto p : page) {
            auto res = p->alloc(sz);
            if (res) return res;
        }
        if (page.size() > 0 && !grow) return nullptr;
        auto p = new Page();
        registerPage(p);
        auto n = p->alloc(sz);
        assert(n);
        return n;
    }

    inline RVal* find(void* ptr) {
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);

        assert(minAddr < maxAddr);

        // quickly discard implausible pointers
        if (addr < minAddr || addr > maxAddr)
            return nullptr;

        for (index_t i = 0; i < page.size(); i++) {
            if (page[i]->isValidObj(ptr)) {
                return (RVal*)ptr;
            }
        }

        return nullptr;
    }

    void sweep() {
        for (auto pi = page.begin(); pi != page.end(); ) {
            auto p = *pi;
            p->sweep();
            // If a page is completely empty we release it
            if (p->empty()) {
#ifdef GC_DEBUG
                std::cout << "Released a Page\n";
#endif
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
#ifdef GC_DEBUG
      std::cout << "Allocated a new Page\n";
#endif
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
    static RVal* alloc(size_t sz, Type type) {
        return inst().doAlloc(sz, type);
    }

private:
    Arena arena;

    RVal* doAlloc(size_t sz, Type type) {
        RVal* res = arena.alloc(sz, false);
        if (!res) {
            doGc();
            res = arena.alloc(sz, false);
            if (!res) res = arena.alloc(sz, true);
        }
        assert(res);
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

    void mark(RVal* val) {
        if (val->mark == 1)
            return;
        assert(val->mark == 0);
        val->mark = 1;
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
    friend void scanStack_();
};

} // namespace gc


#endif

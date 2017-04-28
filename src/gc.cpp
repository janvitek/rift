#include "gc.h"
#include "runtime.h"

namespace gc {

void GarbageCollector::visitChildren(RVal* val) {
    switch (val->type) {
        case Type::Environment: {
            Environment* env = (Environment*)val;
            if (env->bindings)
                mark(env->bindings);
            if (env->parent)
                mark(env->parent);
            break;
        }

        case Type::Bindings: {
            Bindings* env = (Bindings*)val;
            for (unsigned i = 0; i < env->size; ++i) {
                mark(env->binding[i].value);
            }
            break;
        }


        case Type::Function: {
            RFun* fun = (RFun*)val;
            if (fun->args)
                mark(fun->args);
            if (fun->env)
                mark(fun->env);
            break;
        }

        case Type::FunctionArgs:
        case Type::Double:
        case Type::Character:
            // leaf nodes
            break;

        default:
            assert(false && "Broken RVal");
    }
}

// The core mark & sweep algorithm
void GarbageCollector::doGc() {
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

// The stack scan traverses the memory of the C stack and looks at every
// possible stack slot. If we find a valid heap pointer we mark the
// object as well as all objects reachable through it as live.
void __attribute__((noinline)) scanStack_() {
    GarbageCollector& gc = GarbageCollector::inst();

    void ** p = (void**)__builtin_frame_address(0);

#ifdef GC_DEBUG
    unsigned found = 0;
#endif
    while (p < gc.BOTTOM_OF_STACK) {
        uintptr_t po = (uintptr_t)*p;
        if (po > 1024 && po % 4 == 0 && gc.arena.find(*p)) {
#ifdef GC_DEBUG
            found++;
#endif
            gc.mark((RVal*)*p);
        }
        p++;
    }

#ifdef GC_DEBUG
    void ** pt = (void**)__builtin_frame_address(0);
    printf("scanned %lu slots, found %u objs\n", (p-pt)/sizeof(void*), found);
#endif
}


void GarbageCollector::scanStack() {
    // Clobber all registers:
    // -> forces all variables currently hold in registers to be spilled
    //    to the stack where our stackScan can find them.
    __asm__ __volatile__("nop" : :
        : "%rax", "%rbx", "%rcx", "%rdx", "%rsi", "%rdi",
        "%r8", "%r9", "%r10", "%r11", "%r12",
        "%r13", "%r14", "%r15");
    scanStack_();
}

void GarbageCollector::mark() {
    // TODO: maybe some mechanism to define static roots?
    scanStack();
}

void Page::verify() {
    size_t foundFree = 0;
    Free* f = freelist.next;
    while (f) {
        auto b = getIndex(f);
        for (auto i = b; i < b+f->blocks; i++)
           assert(!objSize[i]);
        foundFree += f->blocks * blockSize;
        f = f->next;
    }
    assert(foundFree == freeSpace);

    for (index_t i = 0; i < pageSize; ++i) {
        if (objSize[i]) {
            for (auto c = i+1; c < i+objSize[i]; c++)
              assert(!objSize[c]);
            RVal* o __attribute__((unused)) = getAt(i);
            assert(o->type > Type::Invalid && o->type < Type::End);
            assert(o->mark == 0 || o->mark == 1);
        }
    }
}


}

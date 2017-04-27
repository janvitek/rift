#include "gc.h"
#include "runtime.h"

// The stack scan traverses the memory of the C stack and looks at every
// possible stack slot. If we find a valid heap pointer we mark the
// object as well as all objects reachable through it as live.
void __attribute__((noinline)) gc_scanStack() {
    void ** p = (void**)__builtin_frame_address(0);
    gc::GarbageCollector & inst = gc::GarbageCollector::inst();

    while (p < inst.BOTTOM_OF_STACK) {
        if ((uintptr_t)p > 1024)
            inst.markMaybe(*p);
        p++;
    }
}

namespace gc {

void GarbageCollector::visitChildren(RVal* val) {
    switch (val->type) {
        case Type::Environment: {
            Environment* env = (Environment*)val;
            for (unsigned i = 0; i < env->size; ++i) {
                mark(env->bindings[i].value);
            }
            if (env->parent)
                mark(env->parent);
            break;
        }

        case Type::Function: {
            RFun* fun = (RFun*)val;
            if (fun->env)
                mark(fun->env);
            break;
        }

        case Type::Double:
        case Type::Character:
            // leaf nodes
            break;

        default:
            assert(false && "Broken RVal");
    }
}

}

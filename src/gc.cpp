#include "gc.h"
#include "runtime.h"

// The stack scan traverses the memory of the C stack and looks at every
// possible stack slot. If we find a valid heap pointer we mark the
// object as well as all objects reachable through it as live.
void __attribute__((noinline)) _scanStack() {
    void ** p = (void**)__builtin_frame_address(0);
    gc::GarbageCollector & inst = gc::GarbageCollector::inst();

    while (p < inst.BOTTOM_OF_STACK) {
        if (inst.maybePointer(*p))
            inst.markMaybe(*p);
        p++;
    }
}


namespace gc {



/*
 * Type specific parts
 */

template<>
void GarbageCollector::markImpl<RFun>(RFun * fun) {
    mark(fun->env);
}

template<>
void GarbageCollector::markImpl(DoubleVector *) {
    // Leaf node, nothing to do
}

template<>
void GarbageCollector::markImpl(CharacterVector *) {
    // Leaf node, nothing to do
}

template<>
void GarbageCollector::markImpl<Environment>(Environment * env) {
    for (unsigned i = 0; i < env->size; ++i) {
        auto v = env->bindings[i].value;
        if (auto f = cast<RFun>(v)) {
            mark(f);
        } else if (auto d = cast<DoubleVector>(v)) {
            mark(d);
        } else if (auto c = cast<CharacterVector>(v)) {
            mark(c);
        } else {
            assert(false);
        }
    }
    if (env->parent)
        mark(env->parent);
}

/*
 * Generic GC implementation. Most things are here since they depend on the
 * HEAP_OBJECTS list provided by the runtime. HEAP_OBJECTS is a second order
 * macro which lets us statically loop and generate code for each entry.
 */

size_t GarbageCollector::size() const {
    size_t size = 0;
#define O(T) size += arena<T>().size();
    HEAP_OBJECTS(O);
#undef O
    return size;
}

size_t GarbageCollector::free() const {
    size_t size = 0;
#define O(T) size += arena<T>().free();
    HEAP_OBJECTS(O);
#undef O
    return size;
}

void GarbageCollector::markMaybe(void * ptr) {
#define O(T)                                                            \
    {                                                                   \
        auto i = arena<T>().findNode(ptr);                              \
        if (i.found()) {                                                \
            T * obj = reinterpret_cast<T*>(ptr);                        \
            mark(obj, i);                                               \
        }                                                               \
    }
    HEAP_OBJECTS(O);
#undef O
}

void GarbageCollector::sweep() {
#define O(T) arena<T>().sweep();
    HEAP_OBJECTS(O);
#undef O
}

void GarbageCollector::verify() const {
#define O(T) arena<T>().verify();
    HEAP_OBJECTS(O);
#undef O
}

}

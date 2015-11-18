#include "gc.h"
#include "runtime.h"

namespace gc {

/*
 * Type specific parts
 */

template<>
void GarbageCollector::markImpl<RVal>(RVal * value) {
    if (value->type == RVal::Type::Function)
        mark(value->f);
    // Otherwise leaf node, nothing to do
};

template<>
void GarbageCollector::markImpl<Environment>(Environment * env) {
    for (int i = 0; i < env->size; ++i) {
        mark(env->bindings[i].value);
    }
    if (env->parent)
        mark(env->parent);
}

template<>
void GarbageCollector::markImpl(RFun * fun) {
    mark(fun->env);
};

template<>
void GarbageCollector::markImpl(DoubleVector * value) {
    // Leaf node, nothing to do
};

template<>
void GarbageCollector::markImpl(CharacterVector * value) {
    // Leaf node, nothing to do
};


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

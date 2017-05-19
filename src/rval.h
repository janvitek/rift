#pragma once

#include "rift.h"

/** The type tag associated with values. */
enum class Type : uint8_t {
    Invalid,          // Used for debugging uninitialized tags

    Double,
    Character,
    Function,
    FunctionArgs,
    Environment,
    Bindings,

    End,             // Used to get the max possible value
};

typedef uint8_t Mark;

/** 
 * A Rift Value.
 * These fields and methods are available for all objects.
 */
struct RVal {
    /** Type of this object. */
    Type type;
    /** GC information. */
    Mark mark;

    /** Prints to given stream.  */
    inline void print(ostream & s);

    /** using any of those function will result in a compile-time error.*/
    void* operator new(size_t sz) = delete;
    void* operator new (size_t, void* p) = delete;
    void operator delete(void*) = delete;
    RVal() = delete;
    RVal(RVal const&) = delete;
};

#pragma once

#include "rift.h"

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

/*
 * A Rift Value.
 * All objects start with this
 *
 */
struct RVal {
    Type type;
    Mark mark;

    // deleting new makes sure memory is allocated through the GC
    void* operator new(size_t sz) = delete;
    void* operator new (size_t, void* p) = delete;
    void operator delete(void*) = delete;

    RVal() = delete;
    RVal(RVal const&) = delete;

    /** Prints to given stream.  */
    inline void print(ostream & s);
};

#pragma once
#ifndef RVAL_H
#define RVAL_H

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

/*
 * A Rift Value.
 * All objects start with this
 *
 */
struct RVal {
    Type type;
    uint8_t mark;

    // deleting new makes sure memory is allocated through the GC
    void* operator new(size_t sz) = delete;
    void* operator new (size_t, void* p) = delete;
    void operator delete(void*) = delete;

    RVal() = delete;
    RVal(RVal const&) = delete;

    /** Prints to given stream.  */
    inline void print(std::ostream & s);
};

#endif // RVAL_H

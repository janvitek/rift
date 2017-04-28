#pragma once
#ifndef RVAL_H
#define RVAL_H

enum class Type {
    Invalid,
    Double,
    Character,
    Function,
    FunctionArgs,
    Environment,
    Bindings,
};

/*
 * A Rift Value.
 * All objects start with this
 *
 */
struct RVal {
    Type type;

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

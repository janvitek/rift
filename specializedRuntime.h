#pragma once
#ifndef SPECIALIZED_RUNTIME_H
#define SPECIALIZED_RUNTIME_H

#include "runtime.h"

extern "C" {

    /** Unboxes value to the double vector it contains.
    */
    DoubleVector * doubleFromValue(RVal * v);

    /** Unboxes double vector to the scalar double it contains.
    */
    double scalarFromVector(DoubleVector * v);

    /** Unboxes value to the character vector it contains.
    */
    CharacterVector * characterFromValue(RVal *v);

    /** Unboxes value to the function object it contains.
    */
    RFun * functionFromValue(RVal *v);


    /** Returns a scalar double element from given double vector.
    */
    double doubleGetSingleElement(DoubleVector * from, double index);

    /** Returns a subset of double vector.
    */
    DoubleVector * doubleGetElement(DoubleVector * from, DoubleVector * index);

    /** Returns a subset of character vector.
    */
    CharacterVector * characterGetElement(CharacterVector * from, DoubleVector * index);

    /** Sets the index-th element of given double vector.
    */
    void doubleSetElement(DoubleVector * target, DoubleVector * index, DoubleVector * value);

    /** Sets the specified subset of given double vector.
    */
    void scalarSetElement(DoubleVector * target, double index, double value);

    /** Sets the given subset of character vector.
    */
    void characterSetElement(CharacterVector * target, DoubleVector * index, CharacterVector * value);

    /** Adds two double vectors.
    */
    DoubleVector * doubleAdd(DoubleVector * lhs, DoubleVector * rhs);

    /** Concatenates two character vectors.
    */
    CharacterVector * characterAdd(CharacterVector * lhs, CharacterVector * rhs);

    /** Subtracts two double vectors.
    */
    DoubleVector * doubleSub(DoubleVector * lhs, DoubleVector * rhs);

    /** Multiplies two double vectors.
    */
    DoubleVector * doubleMul(DoubleVector * lhs, DoubleVector * rhs);

    /** Divides two double vectors.
    */
    DoubleVector * doubleDiv(DoubleVector * lhs, DoubleVector * rhs);

    /** Compares the equality of two double vectors.
    */
    DoubleVector * doubleEq(DoubleVector * lhs, DoubleVector * rhs);

    /** Compares the equality of two character vectors.
    */
    DoubleVector * characterEq(CharacterVector * lhs, CharacterVector * rhs);

    /** Compaes the inequality of two double vectors.
    */
    DoubleVector * doubleNeq(DoubleVector * lhs, DoubleVector * rhs);

    /** Compares the inequality of two character vectors.
    */
    DoubleVector * characterNeq(CharacterVector * lhs, CharacterVector * rhs);

    /** Compares two double vectors.
    */
    DoubleVector * doubleLt(DoubleVector * lhs, DoubleVector * rhs);

    /** Compares two double vectors.
    */
    DoubleVector * doubleGt(DoubleVector * lhs, DoubleVector * rhs);

    /** Evaluates given character vector in the specified environment and returns its result.
    */
    RVal * characterEval(Environment * env, CharacterVector * value);

    /** Joins N double vectors together.
    */
    DoubleVector * doublec(int size, ...);

    /** Joins N character vectors together.
    */
    CharacterVector * characterc(int size, ...);

} // extern "C"

#endif // SPECIALIZED_RUNTIME_H

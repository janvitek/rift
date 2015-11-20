#pragma once
#ifndef SPECIALIZED_RUNTIME_H
#define SPECIALIZED_RUNTIME_H

#include "runtime.h"

extern "C" {

    /** Unboxes double vector to the scalar double it contains.
    */
    double scalarFromVector(DoubleVector * v);

    /** Returns a scalar double element from given double vector.
    */
    double doubleGetSingleElement(DoubleVector * from, double index);

    /** Returns a subset of double vector.
    */
    RVal * doubleGetElement(DoubleVector * from, DoubleVector * index);

    /** Returns a subset of character vector.
    */
    RVal * characterGetElement(CharacterVector * from, DoubleVector * index);

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
    RVal * doubleAdd(DoubleVector * lhs, DoubleVector * rhs);

    /** Concatenates two character vectors.
    */
    RVal * characterAdd(CharacterVector * lhs, CharacterVector * rhs);

    /** Subtracts two double vectors.
    */
    RVal * doubleSub(DoubleVector * lhs, DoubleVector * rhs);

    /** Multiplies two double vectors.
    */
    RVal * doubleMul(DoubleVector * lhs, DoubleVector * rhs);

    /** Divides two double vectors.
    */
    RVal * doubleDiv(DoubleVector * lhs, DoubleVector * rhs);

    /** Compares the equality of two double vectors.
    */
    RVal * doubleEq(DoubleVector * lhs, DoubleVector * rhs);

    /** Compares the equality of two character vectors.
    */
    RVal * characterEq(CharacterVector * lhs, CharacterVector * rhs);

    /** Compaes the inequality of two double vectors.
    */
    RVal * doubleNeq(DoubleVector * lhs, DoubleVector * rhs);

    /** Compares the inequality of two character vectors.
    */
    RVal * characterNeq(CharacterVector * lhs, CharacterVector * rhs);

    /** Compares two double vectors.
    */
    RVal * doubleLt(DoubleVector * lhs, DoubleVector * rhs);

    /** Compares two double vectors.
    */
    RVal * doubleGt(DoubleVector * lhs, DoubleVector * rhs);

    /** Evaluates given character vector in the specified environment and returns its result.
    */
    RVal * characterEval(Environment * env, CharacterVector * value);

    /** Joins N double vectors together.
    */
    RVal * doublec(int size, ...);

    /** Joins N character vectors together.
    */
    RVal * characterc(int size, ...);

} // extern "C"

#endif // SPECIALIZED_RUNTIME_H

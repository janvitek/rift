#pragma once
#ifndef RUNTIME_H
#define RUNTIME_H

#include <ciso646>
#include "llvm.h"
#include "ast.h"
#include "objects.h"

#define GENERIC_RUNTIME_FUNCTIONS \
    FUN_PURE(doubleVectorLiteral, type::v_d) \
    FUN_PURE(characterVectorLiteral, type::v_i) \
    FUN_PURE(genericGetElement, type::v_vv) \
    FUN(genericSetElement, type::void_vvv) \
    FUN(envGet, type::v_ei) \
    FUN(envSet, type::void_eiv) \
    FUN_PURE(genericAdd, type::v_vv) \
    FUN_PURE(genericSub, type::v_vv) \
    FUN_PURE(genericMul, type::v_vv) \
    FUN_PURE(genericDiv, type::v_vv) \
    FUN_PURE(genericEq, type::v_vv) \
    FUN_PURE(genericNeq, type::v_vv) \
    FUN_PURE(genericLt, type::v_vv) \
    FUN_PURE(genericGt, type::v_vv) \
    FUN_PURE(createFunction, type::v_ie) \
    FUN_PURE(toBoolean, type::b_v) \
    FUN(call, type::v_viVA) \
    FUN_PURE(length, type::d_v) \
    FUN_PURE(type, type::v_v) \
    FUN(genericEval, type::v_ev) \
    FUN_PURE(c, type::v_iVA)

#if VERSION <= 10
#define RUNTIME_FUNCTIONS GENERIC_RUNTIME_FUNCTIONS
#endif //VERSION

#if VERSION > 10
#define RUNTIME_FUNCTIONS \
    GENERIC_RUNTIME_FUNCTIONS \
    FUN_PURE(doubleGetSingleElement, type::d_dvd) \
    FUN_PURE(doubleGetElement, type::v_dvdv) \
    FUN_PURE(characterGetElement, type::v_cvdv) \
    FUN(doubleSetElement, type::void_dvdvdv) \
    FUN(scalarSetElement, type::void_dvdd) \
    FUN(characterSetElement, type::void_cvdvcv) \
    FUN_PURE(doubleAdd, type::v_dvdv) \
    FUN_PURE(characterAdd, type::v_cvcv) \
    FUN_PURE(doubleSub, type::v_dvdv) \
    FUN_PURE(doubleMul, type::v_dvdv) \
    FUN_PURE(doubleDiv, type::v_dvdv) \
    FUN_PURE(doubleEq, type::v_dvdv) \
    FUN_PURE(doubleNeq, type::v_dvdv) \
    FUN_PURE(characterEq, type::v_cvcv) \
    FUN_PURE(characterNeq, type::v_cvcv) \
    FUN_PURE(doubleLt, type::v_dvdv) \
    FUN_PURE(doubleGt, type::v_dvdv) \
    FUN_PURE(doublec, type::v_iVA) \
    FUN_PURE(characterc, type::v_iVA) \
    FUN(characterEval, type::v_ecv) \
    FUN_PURE(scalarFromVector, type::d_dv)
#endif //VERSION

/** Functions are defined "extern C" to avoid exposing C++ name mangling to
    LLVM. Dealing with C++ would make the compiler less readable.

    For better performance, one could obtain the bitcode of these functions
    and inline that at code generation time.
*/
extern "C" {

/** Creates new environment from parent. */
Environment * envCreate(Environment * parent);

/** Get value of symbol in env or its parent. Throw if not found. */
RVal * envGet(Environment * env, int symbol);

/** Binds symbol to the value in env. */
void envSet(Environment * env, int symbol, RVal * value);

/** Creates a double vector from the literal. */
RVal * doubleVectorLiteral(double value);

/** Creates a CV from the literal at cpIndex in the constant pool */
RVal * characterVectorLiteral(int cpIndex);

/** Returns the value at index.  */
RVal * genericGetElement(RVal * from, RVal * index);

/** Sets the value at index.  */
void genericSetElement(RVal * target, RVal * index, RVal * value);

/** Adds doubles and concatenates strings. */
RVal * genericAdd(RVal * lhs, RVal * rhs);

/** Subtracts doubles, or raises an error. */
RVal * genericSub(RVal * lhs, RVal * rhs);

/** Multiplies doubles, or raises an error. */
RVal * genericMul(RVal * lhs, RVal * rhs);

/** Divides  doubles, or raises an error. */
RVal * genericDiv(RVal * lhs, RVal * rhs);

/** Compares values for equality. */
RVal * genericEq(RVal * lhs, RVal * rhs);

/** Compares values for inequality */
RVal * genericNeq(RVal * lhs, RVal * rhs);

/** Compares doubles using '<', or raises an error. */
RVal * genericLt(RVal * lhs, RVal * rhs);

/** Compares doubles using '>', or raises an  errror. */
RVal * genericGt(RVal * lhs, RVal * rhs);

/** Creates a function and binds it to env. Functions are identified by an
    index assigned at compile time.
 */
RVal * createFunction(int index, Environment * env);

/** Given a value, converts it to a boolean. Used in branches. */
bool toBoolean(RVal * value);

/** Calls function with argc arguments.  argc must match the arity of the
    function. A new env is create for the callee and populated by the
    arguments.
 */
RVal * call(RVal * callee, unsigned argc, ...);

/** Returns the length of a vector.  */
double length(RVal * value);

/** Returns the type of a value as a character vector: 'double',
    'character', or 'function'.
 */
RVal * type(RVal * value);

/** Evaluates character vector in env. */
RVal * eval(Environment * env, char const * value);

  /** Evaluates value, raising an error if it is not a characer vector. */
RVal * genericEval(Environment * env, RVal * value);

/** Creates a vector, raising an error if they are not of the same type, or
    if there is a function among them.
 */
RVal * c(int size, ...);
}

#endif // RUNTIME_H

#pragma once
#ifndef COMPILER_H
#define COMPILER_H

#include <ciso646>

#include "ast.h"
#include "llvm.h"
#include "runtime.h"

namespace rift {

/** Declaration of all Rift types, initialized in the cpp file.  */
namespace type {

extern llvm::Type * Void;
extern llvm::Type * Int;
extern llvm::Type * Double;
extern llvm::Type * Character;
extern llvm::Type * Bool;

extern llvm::PointerType * ptrInt;
extern llvm::PointerType * ptrCharacter;
extern llvm::PointerType * ptrDouble;

extern llvm::StructType * DoubleVector;
extern llvm::StructType * CharacterVector;
extern llvm::PointerType * ptrDoubleVector;
extern llvm::PointerType * ptrCharacterVector;

/** Unions in llvm are represented by the longest members, all others are
    obtained by casting.
*/
extern llvm::StructType * Value;
extern llvm::PointerType * ptrValue;

extern llvm::StructType * Binding;
extern llvm::PointerType * ptrBinding;

extern llvm::PointerType * ptrEnvironment;
extern llvm::StructType * Environment;

extern llvm::FunctionType * NativeCode;

extern llvm::StructType * Function;
extern llvm::PointerType * ptrFunction;


/** Declaration of runtime function types. The naming convention is "x_yz"
    where x is the return type of the function and y and z are arguments. VA
    is used for varargs so that the type of the function can be easily
    determined from the name:
      d = double scalar
      dv = double vector *
      i = integer
      cv = character vector *
      e = Environment *
      v = Value *
      f = Function *
  */
extern llvm::FunctionType * dv_d;
extern llvm::FunctionType * cv_i;
extern llvm::FunctionType * v_dv;
extern llvm::FunctionType * v_cv;
extern llvm::FunctionType * v_ev;
extern llvm::FunctionType * v_vv;
extern llvm::FunctionType * v_vvv;
extern llvm::FunctionType * v_vi;
extern llvm::FunctionType * v_viv;
extern llvm::FunctionType * v_ei;
extern llvm::FunctionType * v_ecv;
extern llvm::FunctionType * void_eiv;
extern llvm::FunctionType * dv_dvdv;
extern llvm::FunctionType * d_dvdv;
extern llvm::FunctionType * cv_cvcv;
extern llvm::FunctionType * dv_cvcv;
extern llvm::FunctionType * d_dvd;
extern llvm::FunctionType * cv_cvdv;
extern llvm::FunctionType * v_f;
extern llvm::FunctionType * f_ie;
extern llvm::FunctionType * b_v;
extern llvm::FunctionType * v_viVA;
extern llvm::FunctionType * void_vvv;
extern llvm::FunctionType * void_dvdvdv;
extern llvm::FunctionType * void_dvdd;
extern llvm::FunctionType * void_cvdvcv;
extern llvm::FunctionType * d_v;
extern llvm::FunctionType * cv_v;
extern llvm::FunctionType * v_iVA;
extern llvm::FunctionType * dv_iVA;
extern llvm::FunctionType * cv_iVA;
extern llvm::FunctionType * dv_v;
extern llvm::FunctionType * d_dv;
extern llvm::FunctionType * f_v;

}

/** Rift Module extends Module and provides all Rift runtime functions. */
class RiftModule : public llvm::Module {

private:
    /** Adds attribute readnone (pure function in LLVM) to function.  */
    static llvm::Function * readnone(llvm::Function * f) {
        llvm::AttributeSet as;
	llvm::AttrBuilder b;
	b.addAttribute(llvm::Attribute::ReadNone);
	as = llvm::AttributeSet::get(llvm::getGlobalContext(), 
				     llvm::AttributeSet::FunctionIndex, 
				     b);
        f->setAttributes(as);
        return f;
    }

public:
    RiftModule() :
        llvm::Module("rift", llvm::getGlobalContext()) {}

    /** Shorthand for pure fun.   */
#define DEF_FUN_PURE(name, signature) \
         llvm::Function * name = \
            readnone(llvm::Function::Create(signature, \
                     llvm::Function::ExternalLinkage, \
                     #name, \
                     this))

    /** Shorthand for impure fun.   */
#define DEF_FUN(name, signature) \
      llvm::Function * name = \
         llvm::Function::Create(signature, \
                                llvm::Function::ExternalLinkage, \
                                #name, \
                                this)
    // All Rift runtime functions must be declared: with a symbol and type.
    DEF_FUN_PURE(doubleVectorLiteral, type::dv_d);
    DEF_FUN_PURE(characterVectorLiteral, type::cv_i);
    DEF_FUN_PURE(fromDoubleVector, type::v_dv);
    DEF_FUN_PURE(fromCharacterVector, type::v_cv);
    DEF_FUN_PURE(fromFunction, type::v_f);
    DEF_FUN_PURE(doubleFromValue, type::dv_v);
    DEF_FUN_PURE(scalarFromVector, type::d_dv);
    DEF_FUN_PURE(characterFromValue, type::cv_v);
    DEF_FUN_PURE(functionFromValue, type::f_v);
    DEF_FUN_PURE(doubleGetSingleElement, type::d_dvd);
    DEF_FUN_PURE(doubleGetElement, type::dv_dvdv);
    DEF_FUN_PURE(characterGetElement, type::cv_cvdv);
    DEF_FUN_PURE(genericGetElement, type::v_vv);
    DEF_FUN(doubleSetElement, type::void_dvdvdv);
    DEF_FUN(scalarSetElement, type::void_dvdd);
    DEF_FUN(characterSetElement, type::void_cvdvcv);
    DEF_FUN(genericSetElement, type::void_vvv);
    DEF_FUN(envGet, type::v_ei);
    DEF_FUN(envSet, type::void_eiv);
    DEF_FUN_PURE(doubleAdd, type::dv_dvdv);
    DEF_FUN_PURE(characterAdd, type::cv_cvcv);
    DEF_FUN_PURE(genericAdd, type::v_vv);
    DEF_FUN_PURE(doubleSub, type::dv_dvdv);
    DEF_FUN_PURE(genericSub, type::v_vv);
    DEF_FUN_PURE(doubleMul, type::dv_dvdv);
    DEF_FUN_PURE(genericMul, type::v_vv);
    DEF_FUN_PURE(doubleDiv, type::dv_dvdv);
    DEF_FUN_PURE(genericDiv, type::v_vv);
    DEF_FUN_PURE(doubleEq, type::dv_dvdv);
    DEF_FUN_PURE(characterEq, type::dv_cvcv);
    DEF_FUN_PURE(genericEq, type::v_vv);
    DEF_FUN_PURE(doubleNeq, type::dv_dvdv);
    DEF_FUN_PURE(characterNeq, type::dv_cvcv);
    DEF_FUN_PURE(genericNeq, type::v_vv);
    DEF_FUN_PURE(doubleLt, type::dv_dvdv);
    DEF_FUN_PURE(genericLt, type::v_vv);
    DEF_FUN_PURE(doubleGt, type::dv_dvdv);
    DEF_FUN_PURE(genericGt, type::v_vv);
    DEF_FUN_PURE(createFunction, type::f_ie);
    DEF_FUN_PURE(toBoolean, type::b_v);
    DEF_FUN(call, type::v_viVA);
    DEF_FUN_PURE(length, type::d_v);
    DEF_FUN_PURE(type, type::cv_v);
    DEF_FUN(characterEval, type::v_ecv);
    DEF_FUN(genericEval, type::v_ev);
    DEF_FUN_PURE(doublec, type::dv_iVA);
    DEF_FUN_PURE(characterc, type::cv_iVA);
    DEF_FUN_PURE(c, type::v_iVA);
    DEF_FUN_PURE(doubleDot, type::d_dvdv);
    DEF_FUN_PURE(genericDot, type::v_vv);

};

/** Compiles ast, returns pointer to the compiled native code. */
FunPtr compile(rift::ast::Fun * f);

}

#endif

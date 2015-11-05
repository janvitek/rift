#pragma once
#ifndef COMPILER_H
#define COMPILER_H

#include <ciso646>

#include "ast.h"
#include "llvm.h"
#include "runtime.h"

namespace rift {

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

        // union
        extern llvm::StructType * Value;
        extern llvm::PointerType * ptrValue;


        extern llvm::StructType * Binding;
        extern llvm::PointerType * ptrBinding;

        extern llvm::PointerType * ptrEnvironment;
        extern llvm::StructType * Environment;

        extern llvm::FunctionType * NativeCode;

        extern llvm::StructType * Function;
        extern llvm::PointerType * ptrFunction;


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
        extern llvm::FunctionType * void_eiv;
        extern llvm::FunctionType * dv_dvdv;
        extern llvm::FunctionType * cv_cvcv;
        extern llvm::FunctionType * dv_cvcv;
        extern llvm::FunctionType * d_dvd;
        extern llvm::FunctionType * cv_cvdv;

        extern llvm::FunctionType * v_f;
        extern llvm::FunctionType * f_ie;

        extern llvm::FunctionType * b_v;

        extern llvm::FunctionType * v_veiVA;

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



    class RiftModule : public llvm::Module {
    public:
        RiftModule() :
            llvm::Module("rift", llvm::getGlobalContext()) {}

#define DEF_FUN(name, signature) llvm::Function * name = llvm::Function::Create(signature, llvm::Function::ExternalLinkage, #name, this)
        DEF_FUN(doubleVectorLiteral, type::dv_d);
        DEF_FUN(characterVectorLiteral, type::cv_i);
        DEF_FUN(fromDoubleVector, type::v_dv);
        DEF_FUN(fromCharacterVector, type::v_cv);
        DEF_FUN(fromFunction, type::v_f);
        DEF_FUN(doubleFromValue, type::dv_v);
        DEF_FUN(scalarFromVector, type::d_dv);
        DEF_FUN(characterFromValue, type::cv_v);
        DEF_FUN(functionFromValue, type::f_v);
        DEF_FUN(doubleGetSingleElement, type::d_dvd);
        DEF_FUN(doubleGetElement, type::dv_dvdv);
        DEF_FUN(characterGetElement, type::cv_cvdv);
        DEF_FUN(genericGetElement, type::v_vv);
        DEF_FUN(doubleSetElement, type::void_dvdvdv);
        DEF_FUN(scalarSetElement, type::void_dvdd);
        DEF_FUN(characterSetElement, type::void_cvdvcv);
        DEF_FUN(genericSetElement, type::void_vvv);
        DEF_FUN(envGet, type::v_ei);
        DEF_FUN(envSet, type::void_eiv);
        DEF_FUN(doubleAdd, type::dv_dvdv);
        DEF_FUN(characterAdd, type::cv_cvcv);
        DEF_FUN(genericAdd, type::v_vv);
        DEF_FUN(doubleSub, type::dv_dvdv);
        DEF_FUN(genericSub, type::v_vv);
        DEF_FUN(doubleMul, type::dv_dvdv);
        DEF_FUN(genericMul, type::v_vv);
        DEF_FUN(doubleDiv, type::dv_dvdv);
        DEF_FUN(genericDiv, type::v_vv);
        DEF_FUN(doubleEq, type::dv_dvdv);
        DEF_FUN(characterEq, type::dv_cvcv);
        DEF_FUN(genericEq, type::v_vv);
        DEF_FUN(doubleNeq, type::dv_dvdv);
        DEF_FUN(characterNeq, type::dv_cvcv);
        DEF_FUN(genericNeq, type::v_vv);
        DEF_FUN(genericLt, type::v_vv);
        DEF_FUN(genericGt, type::v_vv);
        DEF_FUN(createFunction, type::f_ie);
        DEF_FUN(toBoolean, type::b_v);
        DEF_FUN(call, type::v_veiVA);
        DEF_FUN(length, type::d_v);
        DEF_FUN(type, type::cv_v);
        DEF_FUN(genericEval, type::v_ev);
        DEF_FUN(doublec, type::dv_iVA);
        DEF_FUN(characterc, type::cv_iVA);
        DEF_FUN(c, type::v_iVA);

    };



FunPtr compile(rift::ast::Fun * f);

}

#endif

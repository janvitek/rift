#pragma once
#include "llvm.h"
#include "runtime.h"
#include "types.h"

namespace rift {

class RiftModule : public llvm::Module {
public:
    RiftModule();

#define DEF_FUN_PURE(NAME, SIGNATURE) \
    llvm::Function * NAME = declarePureFunction(#NAME, SIGNATURE, this)

#define DEF_FUN(NAME, SIGNATURE) \
    llvm::Function * NAME = declareFunction(#NAME, SIGNATURE, this)

    // All Rift runtime functions must be declared: with a symbol and type.
    DEF_FUN_PURE(doubleVectorLiteral, type::v_d);
    DEF_FUN_PURE(characterVectorLiteral, type::v_i);
    DEF_FUN_PURE(scalarFromVector, type::d_dv);
    DEF_FUN_PURE(doubleGetSingleElement, type::d_dvd);
    DEF_FUN_PURE(doubleGetElement, type::v_dvdv);
    DEF_FUN_PURE(characterGetElement, type::v_cvdv);
    DEF_FUN_PURE(genericGetElement, type::v_vv);
    DEF_FUN(doubleSetElement, type::void_dvdvdv);
    DEF_FUN(scalarSetElement, type::void_dvdd);
    DEF_FUN(characterSetElement, type::void_cvdvcv);
    DEF_FUN(genericSetElement, type::void_vvv);
    DEF_FUN(envGet, type::v_ei);
    DEF_FUN(envSet, type::void_eiv);
    DEF_FUN_PURE(doubleAdd, type::v_dvdv);
    DEF_FUN_PURE(characterAdd, type::v_cvcv);
    DEF_FUN_PURE(genericAdd, type::v_vv);
    DEF_FUN_PURE(doubleSub, type::v_dvdv);
    DEF_FUN_PURE(genericSub, type::v_vv);
    DEF_FUN_PURE(doubleMul, type::v_dvdv);
    DEF_FUN_PURE(genericMul, type::v_vv);
    DEF_FUN_PURE(doubleDiv, type::v_dvdv);
    DEF_FUN_PURE(genericDiv, type::v_vv);
    DEF_FUN_PURE(doubleEq, type::v_dvdv);
    DEF_FUN_PURE(characterEq, type::v_cvcv);
    DEF_FUN_PURE(genericEq, type::v_vv);
    DEF_FUN_PURE(doubleNeq, type::v_dvdv);
    DEF_FUN_PURE(characterNeq, type::v_cvcv);
    DEF_FUN_PURE(genericNeq, type::v_vv);
    DEF_FUN_PURE(doubleLt, type::v_dvdv);
    DEF_FUN_PURE(genericLt, type::v_vv);
    DEF_FUN_PURE(doubleGt, type::v_dvdv);
    DEF_FUN_PURE(genericGt, type::v_vv);
    DEF_FUN_PURE(createFunction, type::v_ie);
    DEF_FUN_PURE(toBoolean, type::b_v);
    DEF_FUN(call, type::v_viVA);
    DEF_FUN_PURE(length, type::d_v);
    DEF_FUN_PURE(type, type::v_v);
    DEF_FUN(characterEval, type::v_ecv);
    DEF_FUN(genericEval, type::v_ev);
    DEF_FUN_PURE(doublec, type::v_iVA);
    DEF_FUN_PURE(characterc, type::v_iVA);
    DEF_FUN_PURE(c, type::v_iVA);

private:

    static llvm::Function * declareFunction(char const * name, llvm::FunctionType * signature, llvm::Module * m);

    /** Adds attribute readnone (pure function in LLVM) to function.  */
    static llvm::Function * declarePureFunction(char const * name, llvm::FunctionType * signature, llvm::Module * m);


};


}

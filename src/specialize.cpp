#if VERSION > 10

#include <iostream>


#include "specialize.h"
#include "rift.h"
#include "compiler/compiler.h"
#include "compiler/types.h"

using namespace llvm;

namespace rift {
char Specialize::ID = 0;

#define RUNTIME_CALL(name, ...) CallInst::Create(Compiler::name(m), \
                                                 vector<Value*>({__VA_ARGS__}),\
                                                 "", ins)
#define CALL(name, ...) CallInst::Create(name, vector<Value*>({__VA_ARGS__}), "", ins)
#define CAST(val, t) CastInst::CreatePointerCast(val, type::t, "", ins)

void Specialize::updateDoubleScalar(Value * v) {
    Value * box = RUNTIME_CALL(doubleVectorLiteral, v);
    state().update(box, AType::D1);
    ins->replaceAllUsesWith(box);
}

void Specialize::updateDoubleOp(Function * fun,
                                Value * lhs, Value * rhs,
                                AType * resType) {
    Value * l = CAST(lhs, ptrDoubleVector); 
    Value * r = CAST(rhs, ptrDoubleVector); 
    Value * res = CALL(fun, l, r);
    state().update(res, resType);
    ins->replaceAllUsesWith(res);
}

void Specialize::updateCharOp(Function * fun,
                              Value * lhs, Value * rhs,
                              AType * resType) {
    Value * l = CAST(lhs, ptrCharacterVector); 
    Value * r = CAST(rhs, ptrCharacterVector); 
    Value * res = CALL(fun, l, r);
    state().update(res, resType);
    ins->replaceAllUsesWith(res);
}

void Specialize::genericAdd() {
    // first check if we are dealing with character add
    Value * lhs = ins->getOperand(0);
    Value * rhs = ins->getOperand(1);
    AType * lhsType = state().get(lhs);
    AType * rhsType = state().get(rhs);
    if (lhsType->isDouble() and rhsType->isDouble()) {
        updateDoubleOp(Compiler::doubleAdd(m), lhs, rhs, lhsType->merge(rhsType));
        changed_ = true;
    } else if (lhsType->isCharacter() and rhsType->isCharacter()) {
        Value * l = CAST(lhs, ptrCharacterVector); 
        Value * r = CAST(rhs, ptrCharacterVector); 
        Value * res = RUNTIME_CALL(characterAdd, l, r);
        state().update(res, AType::CV);
        ins->replaceAllUsesWith(res);
        changed_ = true;
    }
}
 
void Specialize::genericArithmetic(Function * fop) {
    Value * lhs = ins->getOperand(0);
    Value * rhs = ins->getOperand(1);
    AType * lhsType = state().get(lhs);
    AType * rhsType = state().get(rhs);
    if (lhsType->isDouble() and rhsType->isDouble()) {
        updateDoubleOp(fop, lhs, rhs, lhsType->merge(rhsType));
        changed_ = true;
    }
}

void Specialize::genericRelational(Function * fop) {
    Value * lhs = ins->getOperand(0);
    Value * rhs = ins->getOperand(1);
    AType * lhsType = state().get(lhs);
    AType * rhsType = state().get(rhs);
    if (lhsType->isDouble() and rhsType->isDouble()) {
        updateDoubleOp(fop, lhs, rhs, lhsType->merge(rhsType));
        changed_ = true;
    }
}

void Specialize::genericComparison(Value * lhs, Value * rhs,
                                   AType * lhsType, AType * rhsType,
                                   Function * dop, Function * cop) {
    if (lhsType->isDouble() and rhsType->isDouble()) {
        updateDoubleOp(dop, lhs, rhs, lhsType->merge(rhsType));
        changed_ = true;
    } else if (lhsType->isCharacter() and rhsType->isCharacter()) {
        updateCharOp(cop, lhs, rhs, AType::DV);
        changed_ = true;
    }
}

void Specialize::genericNeq() {
    Value * lhs = ins->getOperand(0);
    Value * rhs = ins->getOperand(1);
    AType * lhsType = state().get(lhs);
    AType * rhsType = state().get(rhs);

    if (not lhsType->isSimilar(rhsType)) {
        Value * res = ConstantFP::get(Compiler::context(), APFloat(1.0));
        updateDoubleScalar(res);
        changed_ = true;
    } else {
        genericComparison(lhs, rhs, lhsType, lhsType,
            Compiler::doubleNeq(m), Compiler::characterNeq(m));
    }
}


void Specialize::genericEq() {
    Value * lhs = ins->getOperand(0);
    Value * rhs = ins->getOperand(1);
    AType * lhsType = state().get(lhs);
    AType * rhsType = state().get(rhs);

    if (not lhsType->isSimilar(rhsType)) {
        Value * res = ConstantFP::get(Compiler::context(), APFloat(0.0));
        updateDoubleScalar(res);
        changed_ = true;
    } else {
        genericComparison(lhs, rhs, lhsType, lhsType,
            Compiler::doubleEq(m), Compiler::characterEq(m));
    }
}

void Specialize::genericGetElement() {
#if VERSION >= 19 
    Value * src = ins->getOperand(0);
    Value * idx = ins->getOperand(1);
    AType * srcType = state().get(src);
    AType * idxType = state().get(idx);

    if (srcType->isDouble() && idxType->isDouble()) {
        updateDoubleOp(Compiler::doubleGetElement(m), src, idx, AType::DV);
        changed_ = true;
    } else if (srcType->isCharacter() and idxType->isDouble()) {
        src = CAST(src, ptrCharacterVector);
        idx = CAST(idx, ptrDoubleVector);
        Value * res = RUNTIME_CALL(characterGetElement, src, idx);
        state().update(res, AType::CV);
        ins->replaceAllUsesWith(res);
        changed_ = true;
    }
#endif //VERSION
}

void Specialize::genericC() {
    // if all are double, or all are character, we can do special versions
    CallInst * ci = reinterpret_cast<CallInst*>(ins);
    bool canBeDV = true;
    bool canBeCV = true;
    for (unsigned i = 1; i < ci->getNumArgOperands(); ++i) {
        AType * t = state().get(ci->getArgOperand(i));
        canBeDV = canBeDV and t->isDouble();
        canBeCV = canBeCV and t->isCharacter();
        if (not canBeDV and not canBeCV)
            return; // can't do anything
    }
    vector<Value *> args;
    args.push_back(ci->getArgOperand(0)); // size
    for (unsigned i = 1; i < ci->getNumArgOperands(); ++i) {
        Value * a = ci->getOperand(i);
        args.push_back(canBeDV ? CAST(a, ptrDoubleVector) :
                                 CAST(a, ptrCharacterVector));
    }
    Value * res;
    if (canBeDV) {
        res = CallInst::Create(Compiler::doublec(m), args, "", ins);
        state().update(res, AType::DV);
    } else {
        assert (canBeCV);
        res = CallInst::Create(Compiler::characterc(m), args, "", ins);
        state().update(res, AType::CV);
    }
    ins->replaceAllUsesWith(res);
    changed_ = true;
}

void Specialize::genericEval() {
    Value * arg = ins->getOperand(1);
    AType * argType = state().get(arg);
    if (argType->isCharacter()) {
        arg = CAST(arg, ptrCharacterVector);
        Value * res = RUNTIME_CALL(characterEval, ins->getOperand(0), arg);
        state().update(res, AType::T);
        ins->replaceAllUsesWith(res);
        changed_ = true;
    }
}

bool Specialize::runOnFunction(Function & f) {
    m = f.getParent();
    ta = &getAnalysis<TypeAnalysis>();
    changed_ = false;
    for (auto & b : f) {
        auto i = b.begin();
        while (i != b.end()) {
            ins = &*i;
            if (CallInst * ci = dyn_cast<CallInst>(ins)) {
                StringRef s = ci->getCalledFunction()->getName();
                if (s == "genericAdd") {
                    genericAdd();
                } else if (s == "genericSub") {
                    genericArithmetic(Compiler::doubleSub(m));
                } else if (s == "genericMul") {
                    genericArithmetic(Compiler::doubleMul(m));
                } else if (s == "genericDiv") {
                    genericArithmetic(Compiler::doubleDiv(m));
                } else if (s == "genericLt") {
                    genericRelational(Compiler::doubleLt(m));
                } else if (s == "genericGt") {
                    genericRelational(Compiler::doubleGt(m));
                } else if (s == "genericEq") {
                    genericEq();
                } else if (s == "genericNeq") {
                    genericNeq();
                } else if (s == "genericGetElement") {
                    genericGetElement();
                } else if (s == "c") {
                    genericC();
                } else if (s == "genericEval") {
                    genericEval();
                }
            }
            ++i;
        }
    }
    if (DEBUG) {
        cout << "After specialize optimization: --------------------------------" << endl;
        f.dump();
        state().print(cout);
    }
    return changed_;
}


} // namespace rift

#endif //VERSION

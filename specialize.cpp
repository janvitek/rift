
#include <iostream>
#include <ciso646>

#include "specialize.h"
#include "compiler.h"
#include "rift.h"

using namespace std;
using namespace llvm;

namespace rift {
char Specialize::ID = 0;

#define RUNTIME_CALL(name, ...) CallInst::Create(name, std::vector<llvm::Value*>({__VA_ARGS__}), "", ins)
#define CAST(val, t) CastInst::CreatePointerCast(val, type::t, "", ins)

void Specialize::updateDoubleScalar(llvm::Value * newVal) {
    llvm::Value * box = RUNTIME_CALL(m->doubleVectorLiteral, newVal);
    state().update(box, AType::D1, newVal);
    ins->replaceAllUsesWith(box);
}

void Specialize::updateDoubleOp(llvm::Function * fun,
                              llvm::Value * lhs, llvm::Value * rhs,
                              AType * resType) {
    llvm::Value * l = CAST(lhs, ptrDoubleVector); 
    llvm::Value * r = CAST(rhs, ptrDoubleVector); 
    llvm::Value * res = RUNTIME_CALL(fun, l, r);
    state().update(res, resType);
    ins->replaceAllUsesWith(res);
}

void Specialize::updateCharOp(llvm::Function * fun,
                            llvm::Value * lhs, llvm::Value * rhs,
                            AType * resType) {
    llvm::Value * l = CAST(lhs, ptrCharacterVector); 
    llvm::Value * r = CAST(rhs, ptrCharacterVector); 
    llvm::Value * res = RUNTIME_CALL(fun, l, r);
    state().update(res, resType);
    ins->replaceAllUsesWith(res);
}

bool Specialize::genericAdd() {
    // first check if we are dealing with character add
    llvm::Value * lhs = ins->getOperand(0);
    llvm::Value * rhs = ins->getOperand(1);
    AType * lhsType = state().get(lhs);
    AType * rhsType = state().get(rhs);
    if (lhsType->isDouble() and rhsType->isDouble()) {
        updateDoubleOp(m->doubleAdd, lhs, rhs, lhsType->merge(rhsType));
        return true;
    } else if (lhsType->isCharacter() and rhsType->isCharacter()) {
        llvm::Value * l = CAST(lhs, ptrCharacterVector); 
        llvm::Value * r = CAST(rhs, ptrCharacterVector); 
        llvm::Value * res = RUNTIME_CALL(m->characterAdd, l, r);
        state().update(res, AType::CV);
        ins->replaceAllUsesWith(res);
        return true;
    }
    return false;
}
 
bool Specialize::genericArithmetic(llvm::Function * fop) {
    llvm::Value * lhs = ins->getOperand(0);
    llvm::Value * rhs = ins->getOperand(1);
    AType * lhsType = state().get(lhs);
    AType * rhsType = state().get(rhs);
    if (lhsType->isDouble() and rhsType->isDouble()) {
        updateDoubleOp(fop, lhs, rhs, lhsType->merge(rhsType));
        return true;
    }
    return false;
}

bool Specialize::genericRelational(llvm::Function * fop) {
    llvm::Value * lhs = ins->getOperand(0);
    llvm::Value * rhs = ins->getOperand(1);
    AType * lhsType = state().get(lhs);
    AType * rhsType = state().get(rhs);
    if (lhsType->isDouble() and rhsType->isDouble()) {
        updateDoubleOp(fop, lhs, rhs, lhsType->merge(rhsType));
        return true;
    }

    return false;
}

bool Specialize::genericComparison(llvm::Value * lhs, llvm::Value * rhs,
                                 AType * lhsType, AType * rhsType,
                                 llvm::Function * dop, llvm::Function * cop) {
    if (lhsType->isDouble() and rhsType->isDouble()) {
        updateDoubleOp(dop, lhs, rhs, lhsType->merge(rhsType));
        return true;
    }
    if (lhsType->isCharacter() and rhsType->isCharacter()) {
        updateCharOp(cop, lhs, rhs, AType::DV);
        return true;
    }
    return false;
}

bool Specialize::genericNeq() {
    llvm::Value * lhs = ins->getOperand(0);
    llvm::Value * rhs = ins->getOperand(1);
    AType * lhsType = state().get(lhs);
    AType * rhsType = state().get(rhs);

    if (not lhsType->isSimilar(rhsType)) {
        llvm::Value * res = ConstantFP::get(getGlobalContext(), APFloat(1.0));
        updateDoubleScalar(res);
        return true;
    }

    return genericComparison(lhs, rhs, lhsType, lhsType,
            m->doubleNeq, m->characterNeq);
}


bool Specialize::genericEq() {
    llvm::Value * lhs = ins->getOperand(0);
    llvm::Value * rhs = ins->getOperand(1);
    AType * lhsType = state().get(lhs);
    AType * rhsType = state().get(rhs);

    if (not lhsType->isSimilar(rhsType)) {
        llvm::Value * res = ConstantFP::get(getGlobalContext(), APFloat(0.0));
        updateDoubleScalar(res);
        return true;
    }

    return genericComparison(lhs, rhs, lhsType, lhsType,
            m->doubleEq, m->characterEq);
}

bool Specialize::genericGetElement() {
    llvm::Value * src = ins->getOperand(0);
    llvm::Value * idx = ins->getOperand(1);
    AType * srcType = state().get(src);
    AType * idxType = state().get(idx);

    if (srcType->isDouble() && idxType->isDouble()) {
        updateDoubleOp(m->doubleGetElement, src, idx, AType::DV);
        return true;
    } else if (srcType->isCharacter() and idxType->isDouble()) {
        src = CAST(src, ptrCharacterVector);
        idx = CAST(idx, ptrDoubleVector);
        llvm::Value * res = RUNTIME_CALL(m->characterGetElement, src, idx);
        state().update(res, AType::CV);
        ins->replaceAllUsesWith(res);
        return true;
    }
    return false;
}

bool Specialize::genericC() {
    // if all are double, or all are character, we can do special versions
    CallInst * ci = reinterpret_cast<CallInst*>(ins);
    bool canBeDV = true;
    bool canBeCV = true;
    for (unsigned i = 1; i < ci->getNumArgOperands(); ++i) {
        AType * t = state().get(ci->getArgOperand(i));
        canBeDV = canBeDV and t->isDouble();
        canBeCV = canBeCV and t->isCharacter();
        if (not canBeDV and not canBeCV)
            return false;
    }
    std::vector<llvm::Value *> args;
    args.push_back(ci->getArgOperand(0)); // size
    for (unsigned i = 1; i < ci->getNumArgOperands(); ++i) {
        llvm::Value * a = ci->getOperand(i);
        args.push_back(canBeDV ? CAST(a, ptrDoubleVector) :
                                 CAST(a, ptrCharacterVector));
    }
    llvm::Value * res;
    if (canBeDV) {
        res = CallInst::Create(m->doublec, args, "", ins);
        state().update(res, AType::DV);
    } else {
        assert (canBeCV);
        res = CallInst::Create(m->characterc, args, "", ins);
        state().update(res, AType::CV);
    }
    ins->replaceAllUsesWith(res);
    return true;
}

bool Specialize::genericEval() {
    llvm::Value * arg = ins->getOperand(1);
    AType * argType = state().get(arg);
    if (argType->isCharacter()) {
        arg = CAST(arg, ptrCharacterVector);
        llvm::Value * res = RUNTIME_CALL(m->characterEval, ins->getOperand(0), arg);
        state().update(res, AType::T);
        ins->replaceAllUsesWith(res);
        return true;
    }
    return false;
}

bool Specialize::runOnFunction(llvm::Function & f) {
    //std::cout << "running Specialize optimization..." << std::endl;
    m = reinterpret_cast<RiftModule*>(f.getParent());
    ta = &getAnalysis<TypeAnalysis>();
    for (auto & b : f) {
        auto i = b.begin();
        while (i != b.end()) {
            ins = i;
            bool erase = false;
            if (CallInst * ci = dyn_cast<CallInst>(ins)) {
                StringRef s = ci->getCalledFunction()->getName();
                if (s == "genericAdd") {
                    erase = genericAdd();
                } else if (s == "genericSub") {
                    erase = genericArithmetic(m->doubleSub);
                } else if (s == "genericMul") {
                    erase = genericArithmetic(m->doubleMul);
                } else if (s == "genericDiv") {
                    erase = genericArithmetic(m->doubleDiv);
                } else if (s == "genericLt") {
                    erase = genericRelational(m->doubleLt);
                } else if (s == "genericGt") {
                    erase = genericRelational(m->doubleGt);
                } else if (s == "genericEq") {
                    erase = genericEq();
                } else if (s == "genericNeq") {
                    erase = genericNeq();
                } else if (s == "genericGetElement") {
                    erase = genericGetElement();
                } else if (s == "c") {
                    erase = genericC();
                } else if (s == "genericEval") {
                    erase = genericEval();
                }
            }
            if (erase) {
                llvm::Instruction * v = i;
                ++i;
                state().erase(v);
                v->eraseFromParent();
            } else {
                ++i;
            }
        }
    }
    if (DEBUG) {
        cout << "After unboxing optimization: --------------------------------" << endl;
        f.dump();
        state().print(cout);
    }
    return false;
}


} // namespace rift


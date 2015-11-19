
#include <iostream>
#include <ciso646>

#include "unboxing.h"
#include "compiler.h"
#include "rift.h"

using namespace std;
using namespace llvm;

namespace rift {
char Unboxing::ID = 0;



#define RUNTIME_CALL(name, ...) CallInst::Create(name, std::vector<llvm::Value*>({__VA_ARGS__}), "", ins)

void Unboxing::updateDoubleScalar(llvm::Value * newVal) {
    llvm::Value * box = RUNTIME_CALL(m->doubleVectorLiteral, newVal);
    state().update(box, AType::D, newVal);
    ins->replaceAllUsesWith(box);
}

void Unboxing::updateDoubleOp(llvm::Function * fun,
                              llvm::Value * arg1, llvm::Value * arg2,
                              AType * resType) {
    llvm::Value * l = CastInst::CreatePointerCast(arg1, type::ptrDoubleVector, "", ins); 
    llvm::Value * r = CastInst::CreatePointerCast(arg2, type::ptrDoubleVector, "", ins); 
    llvm::Value * res = RUNTIME_CALL(fun, l, r);
    state().update(res, resType);
    ins->replaceAllUsesWith(res);
}

void Unboxing::updateCharOp(llvm::Function * fun,
                            llvm::Value * arg1, llvm::Value * arg2,
                            AType * resType) {
    llvm::Value * l = CastInst::CreatePointerCast(arg1, type::ptrCharacterVector, "", ins); 
    llvm::Value * r = CastInst::CreatePointerCast(arg2, type::ptrCharacterVector, "", ins); 
    llvm::Value * res = RUNTIME_CALL(fun, l, r);
    state().update(res, resType);
    ins->replaceAllUsesWith(res);
}



bool Unboxing::doubleArithmetic(llvm::Value * lhs, llvm::Value * rhs,
                                AType * lhsType, AType * rhsType,
                                llvm::Instruction::BinaryOps op,
                                llvm::Function * fop) {
    assert(lhsType->isDouble() and rhsType->isDouble() and "Doubles expected");
    if (lhsType->isDoubleScalar() and rhsType->isDoubleScalar()) {
        llvm::Value * l = state().getScalarLocation(lhs);
        llvm::Value * r = state().getScalarLocation(rhs);
        if (l && r) {
            llvm::Value * res = BinaryOperator::Create(op, l, r, "", ins);
            updateDoubleScalar(res);
            return true;
        }
    }

    updateDoubleOp(fop, lhs, rhs, lhsType->merge(rhsType));
    return true;
}

bool Unboxing::genericAdd() {
    // first check if we are dealing with character add
    llvm::Value * lhs = ins->getOperand(0);
    llvm::Value * rhs = ins->getOperand(1);
    AType * lhsType = state().get(lhs);
    AType * rhsType = state().get(rhs);
    if (lhsType->isDouble() and rhsType->isDouble()) {
        return doubleArithmetic(lhs, rhs, lhsType, rhsType, Instruction::FAdd, m->doubleAdd);
    } else if (lhsType->isCharacter() and rhsType->isCharacter()) {
        llvm::Value * l = CastInst::CreatePointerCast(lhs, type::ptrCharacterVector, "", ins); 
        llvm::Value * r = CastInst::CreatePointerCast(rhs, type::ptrCharacterVector, "", ins); 
        llvm::Value * res = RUNTIME_CALL(m->characterAdd, l, r);
        state().update(res, AType::CV);
        ins->replaceAllUsesWith(res);
        return true;
    }
    return false;
}
 
bool Unboxing::genericArithmetic(llvm::Instruction::BinaryOps op, llvm::Function * fop) {
    llvm::Value * lhs = ins->getOperand(0);
    llvm::Value * rhs = ins->getOperand(1);
    AType * lhsType = state().get(lhs);
    AType * rhsType = state().get(rhs);
    if (lhsType->isDouble() and rhsType->isDouble()) {
        return doubleArithmetic(lhs, rhs, lhsType, rhsType, op, fop);
    } else {
        return false;
    }
}

bool Unboxing::doubleRelational(llvm::Value* lhs, llvm::Value * rhs,
                                AType * lhsType, AType * rhsType,
                                llvm::CmpInst::Predicate op, llvm::Function * fop) {
    assert(lhsType->isDouble() and rhsType->isDouble() and "Doubles expected");

    if (lhsType->isDoubleScalar() and rhsType->isDoubleScalar()) {
        llvm::Value * l = state().getScalarLocation(lhs);
        llvm::Value * r = state().getScalarLocation(rhs);
        if (l && r) {
            llvm::Value * x = new FCmpInst(ins, op, l, r);
            llvm::Value * res = new UIToFPInst(x, type::Double, "", ins);
            updateDoubleScalar(res);
            return true;
        }
    }

    updateDoubleOp(fop, lhs, rhs, lhsType->merge(rhsType));
    return true;
}

bool Unboxing::genericRelational(llvm::CmpInst::Predicate op, llvm::Function * fop) {
    llvm::Value * lhs = ins->getOperand(0);
    llvm::Value * rhs = ins->getOperand(1);
    AType * lhsType = state().get(lhs);
    AType * rhsType = state().get(rhs);
    if (lhsType->isDouble() and rhsType->isDouble()) {
        return doubleRelational(lhs, rhs, lhsType, rhsType, op, fop);
    }

    return false;
}

bool Unboxing::genericComparison(llvm::Value * lhs, llvm::Value * rhs,
                                 AType * lhsType, AType * rhsType,
                                 llvm::CmpInst::Predicate op, llvm::Function * dop, llvm::Function * cop) {
    if (lhsType->isDouble() and rhsType->isDouble()) {
        doubleRelational(lhs, rhs, lhsType, rhsType, op, dop);
        return true;
    }
    if (lhsType->isCharacter() and rhsType->isCharacter()) {
        updateCharOp(cop, lhs, rhs, AType::DV);
        return true;
    }
    return false;
}

bool Unboxing::genericNeq() {
    llvm::Value * lhs = ins->getOperand(0);
    llvm::Value * rhs = ins->getOperand(1);
    AType * lhsType = state().get(lhs);
    AType * rhsType = state().get(rhs);

    if (not lhsType->isSimilar(rhsType)) {
        llvm::Value * res = ConstantFP::get(getGlobalContext(), APFloat(1.0));
        updateDoubleScalar(res);
        return true;
    }

    return genericComparison(lhs, rhs, lhsType, lhsType, FCmpInst::FCMP_ONE, m->doubleEq, m->characterEq);
}


bool Unboxing::genericEq() {
    llvm::Value * lhs = ins->getOperand(0);
    llvm::Value * rhs = ins->getOperand(1);
    AType * lhsType = state().get(lhs);
    AType * rhsType = state().get(rhs);

    if (not lhsType->isSimilar(rhsType)) {
        llvm::Value * res = ConstantFP::get(getGlobalContext(), APFloat(0.0));
        updateDoubleScalar(res);
        return true;
    }

    return genericComparison(lhs, rhs, lhsType, lhsType, FCmpInst::FCMP_OEQ, m->doubleEq, m->characterEq);
}

bool Unboxing::genericGetElement() {
    llvm::Value * src = ins->getOperand(0);
    llvm::Value * idx = ins->getOperand(1);
    AType * srcType = state().get(src);
    AType * idxType = state().get(idx);

    if (srcType->isDouble()) {
        src = CastInst::CreatePointerCast(src, type::ptrDoubleVector, "", ins); 
        if (idxType->isDoubleScalar()) {
            llvm::Value * i = state().getScalarLocation(idx);
            if (i) {
                llvm::Value * res = RUNTIME_CALL(m->doubleGetSingleElement, src, i);
                updateDoubleScalar(res);
                return true;
            }
        }
        if (idxType->isDouble()) {
            updateDoubleOp(m->doubleGetElement, src, idx, AType::DV);
            return true;
        }
    } else if (srcType->isCharacter() and idxType->isDouble()) {
        src = CastInst::CreatePointerCast(src, type::ptrCharacterVector, "", ins); 
        idx = CastInst::CreatePointerCast(src, type::ptrDoubleVector, "", ins); 
        llvm::Value * res = RUNTIME_CALL(m->characterGetElement, src, idx);
        state().update(res, AType::CV);
        ins->replaceAllUsesWith(res);
        return true;
    }
    return false;
}

bool Unboxing::genericC() {
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
        args.push_back(canBeDV ?
                CastInst::CreatePointerCast(a, type::ptrDoubleVector, "", ins) :
                CastInst::CreatePointerCast(a, type::ptrCharacterVector, "", ins));
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

bool Unboxing::genericEval() {
    llvm::Value * arg = ins->getOperand(1);
    AType * argType = state().get(arg);
    if (argType->isCharacter()) {
        arg = CastInst::CreatePointerCast(arg, type::ptrCharacterVector, "", ins); 

        llvm::Value * res = RUNTIME_CALL(m->characterEval, ins->getOperand(0), arg);
        state().update(res, AType::T);
        ins->replaceAllUsesWith(res);
        return true;
    }
    return false;
}

bool Unboxing::runOnFunction(llvm::Function & f) {
    //std::cout << "running Unboxing optimization..." << std::endl;
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
                    erase = genericArithmetic(Instruction::FSub, m->doubleSub);
                } else if (s == "genericMul") {
                    erase = genericArithmetic(Instruction::FMul, m->doubleMul);
                } else if (s == "genericDiv") {
                    erase = genericArithmetic(Instruction::FDiv, m->doubleDiv);
                } else if (s == "genericLt") {
                    erase = genericRelational(FCmpInst::FCMP_OLT, m->doubleLt);
                } else if (s == "genericGt") {
                    erase = genericRelational(FCmpInst::FCMP_OGT, m->doubleGt);
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
        cout << state() << endl;
    }
    return false;
}


} // namespace rift


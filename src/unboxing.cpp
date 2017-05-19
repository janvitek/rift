#if VERSION > 10

#include <iostream>


#include "unboxing.h"
#include "rift.h"
#include "compiler/compiler.h"
#include "compiler/types.h"


using namespace std;
using namespace llvm;

namespace rift {

char Unboxing::ID = 0;

#define RUNTIME_CALL(name, ...) CallInst::Create(Compiler::name(m), \
                                                 vector<Value*>({__VA_ARGS__}),\
                                                 "", ins)

void Unboxing::updateDoubleScalar(Value * newVal) {
    Value * box = RUNTIME_CALL(doubleVectorLiteral, newVal);
    state().update(box, AType::D1, newVal);
    ins->replaceAllUsesWith(box);
}

bool Unboxing::doubleArithmetic(Value * lhs, Value * rhs,
                                AType * lhsType, AType * rhsType,
                                Instruction::BinaryOps op) {
    assert(lhsType->isDouble() and rhsType->isDouble() and "Doubles expected");
    if (lhsType->isDoubleScalar() and rhsType->isDoubleScalar()) {
        Value * l = state().getMetadata(lhs);
        Value * r = state().getMetadata(rhs);
        if (l && r) {
            Value * res = BinaryOperator::Create(op, l, r, "", ins);
            updateDoubleScalar(res);
            return true;
        }
    }

    return false;
}

bool Unboxing::genericArithmetic(Instruction::BinaryOps op) {
    Value * lhs = ins->getOperand(0);
    Value * rhs = ins->getOperand(1);
    AType * lhsType = state().get(lhs);
    AType * rhsType = state().get(rhs);
    if (lhsType->isDouble() and rhsType->isDouble()) {
        return doubleArithmetic(lhs, rhs, lhsType, rhsType, op);
    }
    return false;
}

bool Unboxing::doubleRelational(Value* lhs, Value * rhs,
                                AType * lhsType, AType * rhsType,
                                CmpInst::Predicate op) {
    assert(lhsType->isDouble() and rhsType->isDouble() and "Doubles expected");

    if (lhsType->isDoubleScalar() and rhsType->isDoubleScalar()) {
        Value * l = state().getMetadata(lhs);
        Value * r = state().getMetadata(rhs);
        if (l && r) {
            Value * x = new FCmpInst(ins, op, l, r);
            Value * res = new UIToFPInst(x, type::Double, "", ins);
            updateDoubleScalar(res);
            return true;
        }
    }
    return false;
}

bool Unboxing::genericRelational(CmpInst::Predicate op) {
    Value * lhs = ins->getOperand(0);
    Value * rhs = ins->getOperand(1);
    AType * lhsType = state().get(lhs);
    AType * rhsType = state().get(rhs);
    if (lhsType->isDouble() and rhsType->isDouble()) {
        return doubleRelational(lhs, rhs, lhsType, rhsType, op);
    }

    return false;
}


bool Unboxing::genericGetElement() {
    Value * src = ins->getOperand(0);
    Value * idx = ins->getOperand(1);
    AType * srcType = state().get(src);
    AType * idxType = state().get(idx);

    if (srcType->isDouble()) {
        src = CastInst::CreatePointerCast(src, type::ptrDoubleVector, "", ins); 
        if (idxType->isDoubleScalar()) {
            Value * i = state().getMetadata(idx);
            if (i) {
                Value * res = RUNTIME_CALL(doubleGetSingleElement, src, i);
                updateDoubleScalar(res);
                return true;
            }
        }
    }
    return false;
}

bool Unboxing::runOnFunction(Function & f) {
    //cout << "running Unboxing optimization..." << endl;
    m = f.getParent();
    ta = &getAnalysis<TypeAnalysis>();
    for (auto & b : f) {
        auto i = b.begin();
        while (i != b.end()) {
            ins = &*i;
            bool erase = false;
            if (CallInst * ci = dyn_cast<CallInst>(ins)) {
                StringRef s = ci->getCalledFunction()->getName();
                if (s == "genericAdd") {
                    erase = genericArithmetic(Instruction::FAdd);
                } else if (s == "genericSub") {
                    erase = genericArithmetic(Instruction::FSub);
                } else if (s == "genericMul") {
                    erase = genericArithmetic(Instruction::FMul);
                } else if (s == "genericDiv") {
                    erase = genericArithmetic(Instruction::FDiv);
                } else if (s == "genericLt") {
                    erase = genericRelational(FCmpInst::FCMP_OLT);
                } else if (s == "genericGt") {
                    erase = genericRelational(FCmpInst::FCMP_OGT);
                } else if (s == "genericEq") {
                    erase = genericRelational(FCmpInst::FCMP_OEQ);
                } else if (s == "genericNeq") {
                    erase = genericRelational(FCmpInst::FCMP_ONE);
                } else if (s == "genericGetElement") {
                    erase = genericGetElement();
                }
            }
            if (erase) {
                Instruction * v = &*i;
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

#endif //VERSION

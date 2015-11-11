#include <iostream>
#include <ciso646>

#include "unboxing.h"
#include "compiler.h"

using namespace std;
using namespace llvm;

namespace rift {
    char Unboxing::ID = 0;



#define RUNTIME_CALL(name, ...) CallInst::Create(name, std::vector<llvm::Value*>({__VA_ARGS__}), "", ci)

    llvm::Value * Unboxing::unbox(llvm::CallInst * ci, llvm::Value * v) {
        llvm::Value * result = ta->unbox(v);
        if (result == nullptr) {
            Type t = ta->valueType(v);
            if (t == Type::BoxedDoubleScalarVector) {
                result = RUNTIME_CALL(m->doubleFromValue, v);
                ta->setValueType(result, Type::DoubleScalarVector);
            } if (t == Type::BoxedDoubleVector) {
                result = RUNTIME_CALL(m->doubleFromValue, v);
                ta->setValueType(result, Type::DoubleVector);
            } else if (t == Type::DoubleScalarVector) {
                result = RUNTIME_CALL(m->scalarFromVector, v);
                ta->setValueType(result, Type::DoubleScalar);
            } else if (t == Type::BoxedCharacterVector) {
                result = RUNTIME_CALL(m->characterFromValue, v);
                ta->setValueType(result, Type::CharacterVector);
            } else if (t == Type::BoxedRFun) {
                result = RUNTIME_CALL(m->functionFromValue, v);
                ta->setValueType(result, Type::RFun);
            }
            if (result != nullptr)
                ta->setAsBoxed(v, result);
        }
        return result;
    }

    llvm::Value * Unboxing::boxDoubleScalar(CallInst * ci, llvm::Value * scalar) {
        llvm::Value * resultVector = RUNTIME_CALL(m->doubleVectorLiteral, scalar);
        ta->setValueType(resultVector, Type::DoubleScalarVector);
        ta->setAsBoxed(resultVector, scalar);
        return boxDoubleVector(ci, resultVector);
    }

    llvm::Value * Unboxing::boxDoubleVector(CallInst * ci, llvm::Value * vector) {
        llvm::Value * boxed = RUNTIME_CALL(m->fromDoubleVector, vector);
        ta->setValueType(boxed, ta->valueType(vector) == Type::DoubleScalarVector ? Type::BoxedDoubleScalarVector : Type::BoxedDoubleVector);
        ta->setAsBoxed(boxed, vector);
        return boxed;
    }

    llvm::Value * Unboxing::boxCharacterVector(CallInst * ci, llvm::Value * vector) {
        llvm::Value * boxed = RUNTIME_CALL(m->fromCharacterVector, vector);
        ta->setValueType(boxed, Type::BoxedCharacterVector);
        ta->setAsBoxed(boxed, vector);
        return boxed;
    }

    bool Unboxing::doubleArithmetic(llvm::CallInst * ci, llvm::Instruction::BinaryOps op, llvm::Function * fop) {
        llvm::Value * lhs = ci->getOperand(0);
        llvm::Value * rhs = ci->getOperand(1);
        Type tlhs = ta->valueType(lhs);
        Type trhs = ta->valueType(rhs);
        if (tlhs.isScalar() and trhs.isScalar()) {
            lhs = unbox(ci, unbox(ci, lhs));
            rhs = unbox(ci, unbox(ci, rhs));
            llvm::Value * resultScalar = BinaryOperator::Create(op, lhs, rhs, "", ci);
            ta->setValueType(resultScalar, Type::DoubleScalar);
            ci->replaceAllUsesWith(boxDoubleScalar(ci, resultScalar));
            return true;
        } else if (tlhs.isDouble() and trhs.isDouble()) {
            lhs = unbox(ci, lhs);
            rhs = unbox(ci, rhs);
            llvm::Value * resultVector = RUNTIME_CALL(fop, lhs, rhs);
            ta->setValueType(resultVector, Type::DoubleVector);
            ci->replaceAllUsesWith(boxDoubleVector(ci, resultVector));
            return true;
        } else {
            return false;
        }
    }

    bool Unboxing::doubleComparison(CallInst * ci, llvm::CmpInst::Predicate op, llvm::Function * fop) {
        llvm::Value * lhs = ci->getOperand(0);
        llvm::Value * rhs = ci->getOperand(1);
        Type tlhs = ta->valueType(lhs);
        Type trhs = ta->valueType(rhs);
        if (tlhs.isScalar() and trhs.isScalar()) {
            lhs = unbox(ci, unbox(ci, lhs));
            rhs = unbox(ci, unbox(ci, rhs));
            llvm::Value * x = new FCmpInst(ci, op, lhs, rhs);
            llvm::Value * resultScalar = new UIToFPInst(x, type::Double, "", ci);
            ta->setValueType(resultScalar, Type::DoubleScalar);
            ci->replaceAllUsesWith(boxDoubleScalar(ci, resultScalar));
            return true;
        } else if (tlhs.isDouble() and trhs.isDouble()) {
            lhs = unbox(ci, lhs);
            rhs = unbox(ci, rhs);
            llvm::Value * resultVector = RUNTIME_CALL(fop, lhs, rhs);
            ta->setValueType(resultVector, Type::DoubleVector);
            ci->replaceAllUsesWith(boxDoubleVector(ci, resultVector));
            return true;
        } else {
            return false;
        }

    }

    bool Unboxing::characterArithmetic(llvm::CallInst * ci, llvm::Function * fop) {
        llvm::Value * lhs = ci->getOperand(0);
        llvm::Value * rhs = ci->getOperand(1);
        lhs = unbox(ci, lhs);
        rhs = unbox(ci, rhs);
        llvm::Value * resultVector = RUNTIME_CALL(fop, lhs, rhs);
        ta->setValueType(resultVector, Type::CharacterVector);
        ci->replaceAllUsesWith(boxCharacterVector(ci, resultVector));
        return true;

    }

    bool Unboxing::characterComparison(llvm::CallInst * ci, llvm::Function * fop) {
        llvm::Value * lhs = ci->getOperand(0);
        llvm::Value * rhs = ci->getOperand(1);
        lhs = unbox(ci, lhs);
        rhs = unbox(ci, rhs);
        llvm::Value * resultVector = RUNTIME_CALL(fop, lhs, rhs);
        ta->setValueType(resultVector, Type::DoubleVector);
        ci->replaceAllUsesWith(boxDoubleVector(ci, resultVector));
        return true;

    }


    bool Unboxing::genericAdd(CallInst * ci) {
        // first check if we are dealing with character add
        llvm::Value * lhs = ci->getOperand(0);
        llvm::Value * rhs = ci->getOperand(1);
        if (ta->valueType(lhs).isCharacter() and ta->valueType(rhs).isCharacter()) {
            characterArithmetic(ci, m->characterAdd);
            return true;
        } else {
            return doubleArithmetic(ci, Instruction::FAdd, m->doubleAdd);
        }

    }

    bool Unboxing::genericEq(CallInst * ci) {
        llvm::Value * lhs = ci->getOperand(0);
        llvm::Value * rhs = ci->getOperand(1);
        Type tl = ta->valueType(lhs);
        Type tr = ta->valueType(rhs);
        if (tl.isDouble() and tr.isDouble()) {
            return doubleComparison(ci, FCmpInst::FCMP_OEQ, m->doubleEq);
        } else if (tl.isCharacter() and tr.isCharacter()) {
            characterComparison(ci, m->characterEq);
            return true;
        } else if (tl.isDifferentClassAs(tr)) {
            llvm::Value * scalar = ConstantFP::get(getGlobalContext(), APFloat(0.0));
            ta->setValueType(scalar, Type::DoubleScalar);
            ci->replaceAllUsesWith(boxDoubleScalar(ci, scalar));
            return true;
        } else {
            return false;
        }
    }

    bool Unboxing::genericNeq(CallInst * ci) {
        llvm::Value * lhs = ci->getOperand(0);
        llvm::Value * rhs = ci->getOperand(1);
        Type tl = ta->valueType(lhs);
        Type tr = ta->valueType(rhs);
        if (tl.isDouble() and tr.isDouble()) {
            return doubleComparison(ci, FCmpInst::FCMP_ONE, m->doubleNeq);
        } else if (tl.isCharacter() and tr.isCharacter()) {
            characterComparison(ci, m->characterEq);
            return true;
        } else if (tl.isDifferentClassAs(tr)) {
            llvm::Value * scalar = ConstantFP::get(getGlobalContext(), APFloat(1.0));
            ta->setValueType(scalar, Type::DoubleScalar);
            ci->replaceAllUsesWith(boxDoubleScalar(ci, scalar));
            return true;
        } else {
            return false;
        }
    }

    bool Unboxing::genericGetElement(CallInst * ci) {
        llvm::Value * source = ci->getOperand(0);
        llvm::Value * index = ci->getOperand(1);
        Type ts = ta->valueType(source);
        Type ti = ta->valueType(index);
        if (ts.isDouble()) {
            llvm::Value * result;
            if (ti.isScalar()) {
                result = RUNTIME_CALL(m->doubleGetSingleElement, unbox(ci, source), unbox(ci, unbox(ci, index)));
                ta->setValueType(result, Type::DoubleScalar);
                result = boxDoubleScalar(ci, result);
            } else {
                result = RUNTIME_CALL(m->doubleGetElement, unbox(ci, source), unbox(ci, index));
                ta->setValueType(result, Type::DoubleVector);
                result = boxDoubleVector(ci, result);
            }
            ci->replaceAllUsesWith(result);
            return true;
        } else if (ts.isCharacter() and ti.isDouble()) {
            llvm::Value * result = RUNTIME_CALL(m->characterGetElement, unbox(ci, source), unbox(ci, index));
            ta->setValueType(result, Type::CharacterVector);
            ci->replaceAllUsesWith(boxCharacterVector(ci, result));
            return true;
        } else {
            return false;
        }
    }

    bool Unboxing::genericSetElement(CallInst * ci) {
        llvm::Value * target = ci->getOperand(0);
        llvm::Value * index = ci->getOperand(1);
        llvm::Value * value = ci->getOperand(2);
        Type tt = ta->valueType(target);
        Type ti = ta->valueType(index);
        Type tv = ta->valueType(value);
        if (ti.isScalar() and tv.isScalar()) {
            RUNTIME_CALL(m->scalarSetElement, unbox(ci, target), unbox(ci, unbox(ci, index)), unbox(ci, unbox(ci, value)));
            // no need to replace the value, it's void function
            return true;

        } else if (tt.isDouble()) {
            RUNTIME_CALL(m->doubleSetElement, unbox(ci, target), unbox(ci, index), unbox(ci, value));
            // no need to replace the value, it's void function
            return true;
        } else if (tt.isCharacter()) {
            RUNTIME_CALL(m->characterSetElement, unbox(ci, target), unbox(ci, index), unbox(ci, value));
            // no need to replace the value, it's void function
            return true;
        } else {
            return false;
        }
    }


    bool Unboxing::genericC(CallInst * ci) {
        Type t = ta->valueType(ci->getArgOperand(1));
        if (t.isDouble()) {
            for (unsigned i = 2; i < ci->getNumArgOperands(); ++i)
                if (not ta->valueType(ci->getArgOperand(i)).isDouble())
                    return false;
            std::vector<llvm::Value *> args;
            args.push_back(ci->getOperand(0));
            for (unsigned i = 1; i < ci->getNumArgOperands(); ++i)
                args.push_back(unbox(ci, ci->getArgOperand(i)));
            llvm::Value * result = CallInst::Create(m->doublec, args, "", ci);
            ta->setValueType(result, Type::DoubleVector);
            ci->replaceAllUsesWith(boxDoubleVector(ci, result));
            return true;
        } else if (t.isCharacter()) {
            for (unsigned i = 2; i < ci->getNumArgOperands(); ++i)
                if (not ta->valueType(ci->getArgOperand(i)).isCharacter())
                    return false;
            std::vector<llvm::Value *> args;
            args.push_back(ci->getArgOperand(0));
            for (unsigned i = 1; i < ci->getNumArgOperands(); ++i)
                args.push_back(unbox(ci, ci->getOperand(i)));
            llvm::Value * result = CallInst::Create(m->characterc, args, "", ci);
            ta->setValueType(result, Type::CharacterVector);
            ci->replaceAllUsesWith(boxCharacterVector(ci, result));
            return true;
        } else {
            return false;
        }
    }


    bool Unboxing::runOnFunction(llvm::Function & f) {
        //std::cout << "running Unboxing optimization..." << std::endl;
        m = reinterpret_cast<RiftModule*>(f.getParent());
        ta = &getAnalysis<TypeAnalysis>();
        for (auto & b : f) {
            auto i = b.begin();
            while (i != b.end()) {
                bool erase = false;
                if (CallInst * ci = dyn_cast<CallInst>(i)) {
                    StringRef s = ci->getCalledFunction()->getName();
                    if (s == "doubleVectorLiteral") {
                    } else if (s == "genericAdd") {
                        erase = genericAdd(ci);
                    } else if (s == "genericSub") {
                        erase = doubleArithmetic(ci, Instruction::FSub, m->doubleSub);
                    } else if (s == "genericMul") {
                        erase = doubleArithmetic(ci, Instruction::FMul, m->doubleMul);
                    } else if (s == "genericDiv") {
                        erase = doubleArithmetic(ci, Instruction::FDiv, m->doubleDiv);
                    } else if (s == "genericLt") {
                        // types are already checked for us and only doubles can be here
                        erase = doubleComparison(ci, FCmpInst::FCMP_OLT, m->doubleLt);
                    } else if (s == "genericGt") {
                        // types are already checked for us and only doubles can be here
                        erase = doubleComparison(ci, FCmpInst::FCMP_OGT, m->doubleGt);
                    } else if (s == "genericEq") {
                        erase = genericEq(ci);
                    } else if (s == "genericNeq") {
                        erase = genericNeq(ci);
                    } else if (s == "genericGetElement") {
                        erase = genericGetElement(ci);
                    } else if (s == "genericSetElement") {
                        erase = genericSetElement(ci);
                    } else if (s == "c") {
                        erase = genericC(ci);
                    }
                }
                if (erase) {
                    llvm::Instruction * v = i;
                    ++i;
                    v->eraseFromParent();
                } else {
                    ++i;
                }
            }
        }
        //f.dump();
        return false;
    }


} // namespace rift


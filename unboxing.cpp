
#include <iostream>
#include <ciso646>

#include "unboxing.h"
#include "compiler.h"

using namespace std;
using namespace llvm;

namespace rift {
    char Unboxing::ID = 0;



#define RUNTIME_CALL(name, ...) CallInst::Create(name, std::vector<llvm::Value*>({__VA_ARGS__}), "", ins)

    AType * Unboxing::updateAnalysis(llvm::Value * value, AType * type) {
        type->setValue(value);
        ta->setValueType(value, type);
        return type;
    }

    llvm::Value * Unboxing::box(AType * what) {
        AType * t;
        switch (what->kind) {
            case AType::Kind::R:
                return what->loc;
            case AType::Kind::D:
                t = new AType(AType::Kind::DV, what, RUNTIME_CALL(m->doubleVectorLiteral, what->loc));
                ta->setValueType(t->loc, t);
                what = t;
                // fallthrough
            case AType::Kind::DV:
                t = new AType(AType::Kind::R, what, RUNTIME_CALL(m->fromDoubleVector, what->loc));
                ta->setValueType(t->loc, t);
                return t->loc;
            case AType::Kind::CV:
                t = new AType(AType::Kind::R, what, RUNTIME_CALL(m->fromCharacterVector, what->loc));
                ta->setValueType(t->loc, t);
                return t->loc;
            case AType::Kind::F:
                t = new AType(AType::Kind::R, what, RUNTIME_CALL(m->fromFunction, what->loc));
                ta->setValueType(t->loc, t);
                return t->loc;
            default:
                assert(false and "Cannot unbox this type");
                return nullptr;
        }
    }

    llvm:: Value * Unboxing::unbox(AType * t) {
        assert(t->payload != nullptr and "Unboxing requested, but no information is available");
        assert(t->loc != nullptr and "Cannort unbox value with unknown location");
        // payload's location is not known, we have to extract it from the boxed type
        if (not t->hasPayloadLocation()) {
            switch (t->kind) {
                case AType::Kind::DV:
                    t->payload->loc = RUNTIME_CALL(m->doubleGetSingleElement, t->loc, ConstantFP::get(getGlobalContext(), APFloat(0.0)));
                    break;
                case AType::Kind::R:
                    switch (t->payload->kind) {
                        case AType::Kind::DV:
                            t->payload->loc = RUNTIME_CALL(m->doubleFromValue, t->loc);
                            break;
                        case AType::Kind::CV:
                            t->payload->loc = RUNTIME_CALL(m->characterFromValue, t->loc);
                            break;
                        case AType::Kind::F:
                            t->payload->loc = RUNTIME_CALL(m->functionFromValue, t->loc);
                            break;
                        default:
                            assert(false and "Not possible");
                    }
                    ta->setValueType(t->payload->loc, t->payload);
                    break;
                default:
                    assert(false and "Only double vector and RVals can be unboxed");
            }
        }
        return t->payload->loc;
    }

    llvm::Value * Unboxing::getScalarPayload(AType * t) {
        assert (t->loc != nullptr and "This would mean we are calling runtime funtcion with a value that we have effectively lost");
        switch (t->kind) {
            case AType::Kind::D:
                return t->loc;
            case AType::Kind::DV:
                assert(t->payload != nullptr and "Not a scalar");
                return unbox(t);
            case AType::Kind::R:
                assert(t->payload != nullptr and "Not a scalar");
                assert(t->payload->kind == AType::Kind::DV and "Not a scalar");
                // first unboxing to double vector
                unbox(t); 
                // second unboxing to scalar
                return unbox(t->payload);
            default:
                assert(t->payload != nullptr and "Not a scalar");
                return nullptr;
        }
    }

    /** This works for both cv's and dv's. 
     */
    llvm::Value * Unboxing::getVectorPayload(AType * t) {
        assert(t->loc != nullptr and "This would mean we are calling runtime funtcion with a value that we have effectively lost");
        switch (t->kind) {
            case AType::Kind::DV:
            case AType::Kind::CV:
                return t->loc;
            case AType::Kind::R:
                assert(t->payload != nullptr and "Not a vector");
                return unbox(t);
            default:
                assert(t->payload != nullptr and "Not a vector");
                return nullptr;
        }
    }

    void Unboxing::doubleArithmetic(AType * lhs, AType * rhs, llvm::Instruction::BinaryOps op, llvm::Function * fop) {
        assert(lhs->isDouble() and rhs->isDouble() and "Doubles expected");
        AType * result_t;
        if (lhs->isScalar() and rhs->isScalar()) {
            result_t = updateAnalysis(BinaryOperator::Create(op, getScalarPayload(lhs), getScalarPayload(rhs), "", ins), new AType(AType::Kind::D));
        } else {
            // it has to be vector - we have already checked it is a double
            result_t = updateAnalysis(RUNTIME_CALL(fop, getVectorPayload(lhs), getVectorPayload(rhs)), new AType(AType::Kind::DV));
        }
        ins->replaceAllUsesWith(box(result_t));
    }

    bool Unboxing::genericAdd() {
        // first check if we are dealing with character add
        AType * lhs = ta->valueType(ins->getOperand(0));
        AType * rhs = ta->valueType(ins->getOperand(1));
        if (lhs->isDouble() and rhs->isDouble()) {
            doubleArithmetic(lhs, rhs, Instruction::FAdd, m->doubleAdd);
            return true;
        } else if (lhs->isCharacter() and rhs->isCharacter()) {
            AType * result_t = updateAnalysis(RUNTIME_CALL(m->characterAdd, getVectorPayload(lhs), getVectorPayload(rhs)), new AType(AType::Kind::CV));
            ins->replaceAllUsesWith(box(result_t));
            return true;
        } else {
            return false;
        }
    }

    bool Unboxing::genericArithmetic(llvm::Instruction::BinaryOps op, llvm::Function * fop) {
        AType * lhs = ta->valueType(ins->getOperand(0));
        AType * rhs = ta->valueType(ins->getOperand(1));
        if (lhs->isDouble() and rhs->isDouble()) {
            doubleArithmetic(lhs, rhs, op, fop);
            return true;
        } else {
            return false;
        }
    }

    void Unboxing::doubleRelational(AType * lhs, AType * rhs, llvm::CmpInst::Predicate op, llvm::Function * fop) {
        assert(lhs->isDouble() and rhs->isDouble() and "Doubles expected");
        AType * result_t;
        if (lhs->isScalar() and rhs->isScalar()) {
            llvm::Value * x = new FCmpInst(ins, op, getScalarPayload(lhs), getScalarPayload(rhs));
            result_t = updateAnalysis(new UIToFPInst(x, type::Double, "", ins), new AType(AType::Kind::D));
        } else {
            // it has to be vector - we have already checked it is a double
            result_t = updateAnalysis(RUNTIME_CALL(fop, getVectorPayload(lhs), getVectorPayload(rhs)), new AType(AType::Kind::DV));
        }
        ins->replaceAllUsesWith(box(result_t));

    }

    bool Unboxing::genericRelational(llvm::CmpInst::Predicate op, llvm::Function * fop) {
        AType * lhs = ta->valueType(ins->getOperand(0));
        AType * rhs = ta->valueType(ins->getOperand(1));
        if (lhs->isDouble() and rhs->isDouble()) {
            doubleRelational(lhs, rhs, op, fop);
            return true;
        } else {
            return false;
        }
    }

    bool Unboxing::genericComparison(AType * lhs, AType * rhs, llvm::CmpInst::Predicate op, llvm::Function * dop, llvm::Function * cop) {
        if (lhs->isDouble() and rhs->isDouble()) {
            doubleRelational(lhs, rhs, op, dop);
            return true;
        } else if (lhs->isCharacter() and rhs->isCharacter()) {
            AType * result_t = updateAnalysis(RUNTIME_CALL(cop, getVectorPayload(lhs), getVectorPayload(rhs)), new AType(AType::Kind::DV));
            ins->replaceAllUsesWith(box(result_t));
            return true;
        } else {
            return false;
        }
    }

    bool Unboxing::genericEq() {
        AType * lhs = ta->valueType(ins->getOperand(0));
        AType * rhs = ta->valueType(ins->getOperand(1));
        if (genericComparison(lhs, rhs, FCmpInst::FCMP_OEQ, m->doubleEq, m->characterEq)) {
            return true;
        } else if (not lhs->canBeSameTypeAs(rhs)) {
            AType * result_t = updateAnalysis(ConstantFP::get(getGlobalContext(), APFloat(0.0)), new AType(AType::Kind::D));
            ins->replaceAllUsesWith(box(result_t));
            return true;
        } else {
            return false;
        }

    }
    bool Unboxing::genericNeq() {
        AType * lhs = ta->valueType(ins->getOperand(0));
        AType * rhs = ta->valueType(ins->getOperand(1));
        if (genericComparison(lhs, rhs, FCmpInst::FCMP_ONE, m->doubleNeq, m->characterNeq)) {
            return true;
        } else if (not lhs->canBeSameTypeAs(rhs)) {
            AType * result_t = updateAnalysis(ConstantFP::get(getGlobalContext(), APFloat(1.0)), new AType(AType::Kind::D));
            ins->replaceAllUsesWith(box(result_t));
            return true;
        } else {
            return false;
        }
    }

    bool Unboxing::genericGetElement() {
        AType * source = ta->valueType(ins->getOperand(0));
        AType * index = ta->valueType(ins->getOperand(1));
        AType * result_t;
        if (source->isDouble()) {
            if (index->isScalar()) {
                result_t = updateAnalysis(RUNTIME_CALL(m->doubleGetSingleElement, getVectorPayload(source), getScalarPayload(index)), new AType(AType::Kind::D));
            } else if (index->isDouble()) {
                result_t = updateAnalysis(RUNTIME_CALL(m->doubleGetElement, getVectorPayload(source), getVectorPayload(index)), new AType(AType::Kind::DV));
            } else {
                return false;
            }
        } else if (source->isCharacter() and index->isDouble()) {
            result_t = updateAnalysis(RUNTIME_CALL(m->characterGetElement, getVectorPayload(source), getVectorPayload(index)), new AType(AType::Kind::CV));
        } else {
            return false;
        }
        ins->replaceAllUsesWith(box(result_t));
        return true;
    }

    bool Unboxing::genericSetElement() {
        AType * target = ta->valueType(ins->getOperand(0));
        AType * index = ta->valueType(ins->getOperand(1));
        AType * value = ta->valueType(ins->getOperand(2));
        if (target->isDouble()) {
            if (index->isScalar() and value->isScalar())
                RUNTIME_CALL(m->scalarSetElement, getVectorPayload(target), getScalarPayload(index), getScalarPayload(value));
            else if (index->isDouble() and value->isDouble())
                RUNTIME_CALL(m->doubleSetElement, getVectorPayload(target), getVectorPayload(index), getVectorPayload(value));
            else 
                return false;
            return true;
        } else if (target->isCharacter() and index->isDouble() and value->isCharacter()) {
            RUNTIME_CALL(m->characterSetElement, getVectorPayload(target), getVectorPayload(index), getVectorPayload(value));
            return true;
        } else {
            return false;
        }
    }

    bool Unboxing::genericC() {
        // if all are double, or all are character, we can do special versions
        CallInst * ci = reinterpret_cast<CallInst*>(ins);
        bool canBeDV = true;
        bool canBeCV = true;
        std::vector<AType *> args_t;
        for (unsigned i = 1; i < ci->getNumArgOperands(); ++i) {
            AType * t = ta->valueType(ci->getArgOperand(i));
            canBeDV = canBeDV and t->isDouble();
            canBeCV = canBeCV and t->isCharacter();
            if (not canBeDV and not canBeCV)
                return false;
            args_t.push_back(t);
        }
        std::vector<llvm::Value *> args;
        args.push_back(ci->getArgOperand(0)); // size
        for (AType * t : args_t)
            args.push_back(getVectorPayload(t));
        AType * result_t;
        if (canBeDV) {
            result_t = updateAnalysis(CallInst::Create(m->doublec, args, "", ins), new AType(AType::Kind::DV));
        } else {
            assert (canBeCV);
            result_t = updateAnalysis(CallInst::Create(m->characterc, args, "", ins), new AType(AType::Kind::CV));
        }
        ins->replaceAllUsesWith(box(result_t));
        return true;
    }

    bool Unboxing::genericEval() {
        AType * arg =ta->valueType(ins->getOperand(1));
        if (arg->isCharacter()) {
            AType * result_t = updateAnalysis(RUNTIME_CALL(m->characterEval, ins->getOperand(0), getVectorPayload(arg)), new AType(AType::Kind::R));
            ins->replaceAllUsesWith(result_t->loc);
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
                    } else if (s == "genericSetElement") {
                        erase = genericSetElement();
                    } else if (s == "c") {
                        erase = genericC();
                    } else if (s == "genericEval") {
                        erase = genericEval();
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
        //cout << *ta << endl;
        return false;
    }


} // namespace rift


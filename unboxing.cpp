
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

AType * Unboxing::updateAnalysis(llvm::Value * value, AType * type) {
    return state().initialize(value, type);
}

llvm::Value * Unboxing::box(AType * what) {
    llvm::Value * location = state().getLocation(what);
    switch (what->kind) {
        case AType::Kind::R:
            return location;
        case AType::Kind::D:
            location = RUNTIME_CALL(m->doubleVectorLiteral, location);
            what = state().initialize(location, new AType(AType::Kind::DV, what));
            // fallthrough
        case AType::Kind::DV:
            location = RUNTIME_CALL(m->fromDoubleVector, location);
            state().initialize(location, new AType(AType::Kind::R, what));
            return location;
        case AType::Kind::CV:
            location = RUNTIME_CALL(m->fromCharacterVector, location);
            state().initialize(location, new AType(AType::Kind::R, what));
            return location;
        case AType::Kind::F:
            location = RUNTIME_CALL(m->fromFunction, location);
            state().initialize(location, new AType(AType::Kind::R, what));
            return location;
        default:
            assert(false and "Cannot unbox this type");
            return nullptr;
    }
}

llvm:: Value * Unboxing::unbox(AType * t) {
    llvm::Value * location = state().getLocation(t);
    assert(t->payload != nullptr and "Unboxing requested, but no information is available");
    assert(location != nullptr and "Cannort unbox value with unknown location");
    if (llvm::Value * payloadLocation = state().getLocation(t->payload))
        return payloadLocation;

    // payload's location is not known, we have to extract it from the boxed type
    switch (t->kind) {
        case AType::Kind::DV:
            location = RUNTIME_CALL(m->doubleGetSingleElement, location, ConstantFP::get(getGlobalContext(), APFloat(0.0)));
            state().initialize(location, t->payload);
            break;
        case AType::Kind::R:
            switch (t->payload->kind) {
                case AType::Kind::DV:
                    location = RUNTIME_CALL(m->doubleFromValue, location);
                    break;
                case AType::Kind::CV:
                    location = RUNTIME_CALL(m->characterFromValue, location);
                    break;
                case AType::Kind::F:
                    location = RUNTIME_CALL(m->functionFromValue, location);
                    break;
                default:
                    assert(false and "Not possible");
            }
            state().initialize(location, t->payload);
            break;
        default:
            assert(false and "Only double vector and RVals can be unboxed");
    }
    return location;
}

llvm::Value * Unboxing::getScalarPayload(AType * t) {
    llvm::Value* location = state().getLocation(t);
    assert (location != nullptr and "This would mean we are calling runtime funtcion with a value that we have effectively lost");
    switch (t->kind) {
        case AType::Kind::D:
            return location;
        case AType::Kind::DV:
            assert(not t->payload->isTop() and "Not a scalar");
            return unbox(t);
        case AType::Kind::R:
            assert(not t->payload->isTop() and "Not a scalar");
            assert(t->payload->kind == AType::Kind::DV and "Not a scalar");
            // first unboxing to double vector
            unbox(t); 
            // second unboxing to scalar
            return unbox(t->payload);
        default:
            assert(not t->payload->isTop() and "Not a scalar");
            return nullptr;
    }
}

/** This works for both cv's and dv's. 
*/
llvm::Value * Unboxing::getVectorPayload(AType * t) {
    llvm::Value * location = state().getLocation(t);
    assert(location != nullptr and "This would mean we are calling runtime funtcion with a value that we have effectively lost");
    switch (t->kind) {
        case AType::Kind::DV:
        case AType::Kind::CV:
            return location;
        case AType::Kind::R:
            assert(not t->payload->isTop() and "Not a vector");
            return unbox(t);
        default:
            assert(not t->payload->isTop() and "Not a vector");
            return nullptr;
    }
}

void Unboxing::doubleArithmetic(AType * lhs, AType * rhs, llvm::Instruction::BinaryOps op, llvm::Function * fop) {
    assert(lhs->isDouble() and rhs->isDouble() and "Doubles expected");
    AType * result_t;
    if (lhs->isScalar() and rhs->isScalar()) {
        llvm::Value * l = getScalarPayload(lhs);
        llvm::Value * r = getScalarPayload(rhs);
        result_t = updateAnalysis(
                BinaryOperator::Create(op, l, r, "", ins),
                new AType(AType::Kind::D));
    } else {
        // it has to be vector - we have already checked it is a double
        result_t = updateAnalysis(
                RUNTIME_CALL(fop, getVectorPayload(lhs), getVectorPayload(rhs)),
                new AType(AType::Kind::DV));
    }
    ins->replaceAllUsesWith(box(result_t));
}

bool Unboxing::genericAdd() {
    // first check if we are dealing with character add
    AType * lhs = state().get(ins->getOperand(0));
    AType * rhs = state().get(ins->getOperand(1));
    if (lhs->isDouble() and rhs->isDouble()) {
        doubleArithmetic(lhs, rhs, Instruction::FAdd, m->doubleAdd);
        return true;
    } else if (lhs->isCharacter() and rhs->isCharacter()) {
        AType * result_t = updateAnalysis(
                RUNTIME_CALL(m->characterAdd, getVectorPayload(lhs), getVectorPayload(rhs)),
                new AType(AType::Kind::CV));
        ins->replaceAllUsesWith(box(result_t));
        return true;
    } else {
        return false;
    }
}

bool Unboxing::genericArithmetic(llvm::Instruction::BinaryOps op, llvm::Function * fop) {
    AType * lhs = state().get(ins->getOperand(0));
    AType * rhs = state().get(ins->getOperand(1));
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
    AType * lhs = state().get(ins->getOperand(0));
    AType * rhs = state().get(ins->getOperand(1));
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
    AType * lhs = state().get(ins->getOperand(0));
    AType * rhs = state().get(ins->getOperand(1));
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
    AType * lhs = state().get(ins->getOperand(0));
    AType * rhs = state().get(ins->getOperand(1));
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
    AType * source = state().get(ins->getOperand(0));
    AType * index = state().get(ins->getOperand(1));
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
    AType * target = state().get(ins->getOperand(0));
    AType * index = state().get(ins->getOperand(1));
    AType * value = state().get(ins->getOperand(2));
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
        AType * t = state().get(ci->getArgOperand(i));
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
    AType * arg =state().get(ins->getOperand(1));
    if (arg->isCharacter()) {
        AType * result_t = updateAnalysis(
                RUNTIME_CALL(m->characterEval, ins->getOperand(0), getVectorPayload(arg)),
                new AType(AType::Kind::R));
        ins->replaceAllUsesWith(state().getLocation(result_t));
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


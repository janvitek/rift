#if VERSION > 10
#include <iostream>

#include "type_analysis.h"
#include "rift.h"

using namespace llvm;

namespace rift {
/// Initializing our singletons...
AType * AType::T  = new AType("R");
AType * AType::D1 = new AType("D1");
AType * AType::DV = new AType("DV");
AType * AType::CV = new AType("CV");
AType * AType::F  = new AType("F");
AType * AType::B  = new AType("??");

/// ID is required by LLVM
char TypeAnalysis::ID = 0;

void TypeAnalysis::genericArithmetic(CallInst * ci) {
    AType * lhs = state.get(ci->getOperand(0));
    AType * rhs = state.get(ci->getOperand(1));
    state.update(ci, lhs->lub(rhs));
}

void TypeAnalysis::genericRelational(CallInst * ci) {
    AType * lhs = state.get(ci->getOperand(0));
    AType * rhs = state.get(ci->getOperand(1));
    if (lhs->isDoubleScalar() and rhs->isDoubleScalar()) {
        state.update(ci, AType::D1);
    } else {
        state.update(ci, AType::DV);
    }
}

void TypeAnalysis::genericGetElement(CallInst * ci) {
    AType * from = state.get(ci->getOperand(0));
    AType * index = state.get(ci->getOperand(1));
    if (from->isDouble()) {
        if (index->isDoubleScalar()) {
            state.update(ci, AType::D1);
        } else {
            state.update(ci, AType::DV);
        }
    } else if (from->isCharacter()) {
        state.update(ci, AType::CV);
    } else {
        state.update(ci, AType::T);
    }
}

void TypeAnalysis::analyzeCallInst(CallInst* ci, StringRef s) {
    if (s == "doubleVectorLiteral") {
        state.update(ci, AType::D1);
#if VERSION >= 18
    } else if (s == "characterVectorLiteral") {
        state.update(ci, AType::CV);
#endif //VERSION
    } else if (s == "genericGetElement") {
        genericGetElement(ci);
    } else if (s == "genericSetElement") {
        // don't do anything for set as it does not produce any new value
    } else if (s=="genericAdd" || s=="genericSub" || s=="genericMul" || s=="genericDiv") {
        genericArithmetic(ci);
    } else if (s=="genericEq" || s=="genericNeq" || s=="genericLt" || s=="genericGt") {
        genericRelational(ci);
    } else if (s == "length") {
        // length() returns a scalar
        state.update(ci, AType::D1);
    } else if (s == "type") {
#if VERSION < 18
        // TODO
#endif //VERSION
#if VERSION >= 18
        // type() returns a character vector
        state.update(ci, AType::CV);
#endif //VERSION
    } else if (s == "c") {
        // make sure the types to c are correct
        AType * t1 = state.get(ci->getArgOperand(1));
        for (unsigned i = 2; i < ci->getNumArgOperands(); ++i)
            t1 = t1->lub(state.get(ci->getArgOperand(i)));
        if (t1->isDoubleScalar())// concatenation of scalars is a vector
            t1 = AType::DV;
        state.update(ci, t1);
    } else if (s == "genericEval" || s == "envGet") {
        state.update(ci, AType::T);
    } else {
        state.update(ci, AType::T);
    }
}

bool TypeAnalysis::runOnFunction(llvm::Function & f) {
    state.clear();
    if (DEBUG) cout << "runnning TypeAnalysis..." << endl;
    do { // for all basic blocks, for all instructions
        state.iterationStart();
        for (auto & b : f) {
            for (auto & i : b) {
                if (CallInst * ci = dyn_cast<CallInst>(&i)) {
                    StringRef s = ci->getCalledFunction()->getName();
                    analyzeCallInst(ci, s);
                } else if (PHINode * phi = dyn_cast<PHINode>(&i)) {
                    AType * l = state.get(phi->getOperand(0));
                    AType * r = state.get(phi->getOperand(1));
                    state.update(phi, l->lub(r));
                } else {
                    // ignore control flow operations
                }
            }
        }
    } while (!state.hasReachedFixpoint());
    if (DEBUG) { f.dump(); state.print(cout);  }
    return false;
}

/// Debug
ostream & operator << (ostream & s, AType & t) {
    s << t.name;
    return s;
}
} // namespace rift

#endif //VERSION

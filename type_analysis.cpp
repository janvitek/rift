#include <iostream>
#include <ciso646>
#include <set>

#include "type_analysis.h"
#include "compiler.h"
#include "rift.h"

using namespace std;
using namespace llvm;

namespace rift {

AType * AType::T  = new AType();
AType * AType::B  = new AType();
AType * AType::D  = new AType();
AType * AType::DV = new AType();
AType * AType::CV = new AType();
AType * AType::F  = new AType();

char TypeAnalysis::ID = 0;

void TypeAnalysis::genericArithmetic(CallInst * ci) {
    AType * lhs = state.get(ci->getOperand(0));
    AType * rhs = state.get(ci->getOperand(1));
    state.update(ci, lhs->merge(rhs));
}

void TypeAnalysis::genericRelational(CallInst * ci) {
    AType * lhs = state.get(ci->getOperand(0));
    AType * rhs = state.get(ci->getOperand(1));
    if (lhs->isDoubleScalar() and rhs->isDoubleScalar()) {
        state.update(ci, AType::D);
    } else {
        state.update(ci, AType::DV);
    }
}


void TypeAnalysis::genericGetElement(CallInst * ci) {
    AType * from = state.get(ci->getOperand(0));
    AType * index = state.get(ci->getOperand(1));
    if (from->isDouble()) {
        if (index->isDoubleScalar()) {
            state.update(ci, AType::D);
        } else {
            state.update(ci, AType::DV);
        }
    } else if (from->isCharacter()) {
        state.update(ci, AType::CV);
    } else {
        state.update(ci, AType::T);
    }
}

bool TypeAnalysis::runOnFunction(llvm::Function & f) {
    state.clear();
    if (DEBUG) std::cout << "runnning TypeAnalysis..." << std::endl;
    // for all basic blocks, for all instructions
    do {
        // cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << endl;
        state.iterationStart();
        for (auto & b : f) {
            for (auto & i : b) {
                if (CallInst * ci = dyn_cast<CallInst>(&i)) {
                    StringRef s = ci->getCalledFunction()->getName();
                    if (s == "doubleVectorLiteral") {
                        // when creating literal from a double, it is
                        // always double scalar
                        llvm::Value * op = ci->getOperand(0);
                        state.update(ci, AType::D, op);
                    } else if (s == "characterVectorLiteral") {
                        state.update(ci, AType::CV);
                    } else if (s == "genericGetElement") {
                        genericGetElement(ci);
                    } else if (s == "genericSetElement") {
                        // don't do anything for set element as it does not
                        // produce any new value
                    } else if (s == "genericAdd") {
                        genericArithmetic(ci);
                    } else if (s == "genericSub") {
                        genericArithmetic(ci);
                    } else if (s == "genericMul") {
                        genericArithmetic(ci);
                    } else if (s == "genericDiv") {
                        genericArithmetic(ci);
                    } else if (s == "genericEq") {
                        genericRelational(ci);
                    } else if (s == "genericNeq") {
                        genericRelational(ci);
                    } else if (s == "genericLt") {
                        genericRelational(ci);
                    } else if (s == "genericGt") {
                        genericRelational(ci);
                    } else if (s == "length") {
                        // result of length operation is always 
                        // double scalar
                        state.update(ci, AType::D);
                    } else if (s == "type") {
                        // result of type operation is always 
                        // character vector
                        state.update(ci, AType::CV);
                    } else if (s == "c") {
                        // make sure the types to c are correct
                        AType * t1 = state.get(ci->getArgOperand(1));
                        for (unsigned i = 2; i < ci->getNumArgOperands(); ++i)
                            t1 = t1->merge(state.get(ci->getArgOperand(i)));
                        if (t1->isDoubleScalar())
                            // concatenation of scalars is a vector
                            t1 = AType::DV;
                        state.update(ci, t1);
                    } else if (s == "genericEval") {
                        state.update(ci, AType::T);
                    } else if (s == "envGet") {
                        state.update(ci, AType::T);
                    }
                } else if (PHINode * phi = dyn_cast<PHINode>(&i)) {
                    AType * first = state.get(phi->getOperand(0));
                    AType * second = state.get(phi->getOperand(1));
                    AType * result = first->merge(second);
                    state.update(phi, result);
                }
            }
        }
    } while (!state.hasReachedFixpoint());
    if (DEBUG) {
        f.dump();
        cout << state << endl;
    }
    return false;
}

std::ostream & operator << (std::ostream & s, AbstractState & m) {
    s << "Abstract State: " << "\n";

    struct cmpByName {
        bool operator()(const llvm::Value * a, const llvm::Value * b) const {
            int aName, bName;

            std::stringstream s;
            llvm::raw_os_ostream ss(s);
            a->printAsOperand(ss, false);
            ss.flush();
            s.seekg(1);
            s >> aName;

            std::stringstream s2;
            llvm::raw_os_ostream ss2(s2);
            b->printAsOperand(ss2, false);
            ss2.flush();
            s2.seekg(1);
            s2 >> bName;

            return aName < bName;
        }
    };

    std::set<llvm::Value*, cmpByName> sorted;
    for (auto const & v : m.type) {
        auto pos = std::get<0>(v);
        sorted.insert(pos);
    }

    for (auto pos : sorted) {
        auto st = m.type.at(pos);

        llvm::raw_os_ostream ss(s);
        pos->printAsOperand(ss, false);
        ss << ": ";
        if (auto loc = m.getScalarLocation(pos))
            loc->printAsOperand(ss, false);
        ss.flush();

        st->print(s, m);
        s << endl;
    }
    return s;
}


void AType::print(std::ostream & s, AbstractState & m) {
    llvm::raw_os_ostream ss(s);
    if (this == D)
        ss << " D ";
    else if (this == DV)
        ss << " DV ";
    else if (this == CV)
        ss << " CV ";
    else if (this == F)
        ss << " F ";
    else if (this == T)
        ss << " T ";
    else if (this == B)
        ss << " ?? ";
    else
        assert(false);

    ss.flush();
}

} // namespace rift


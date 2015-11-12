#include <iostream>
#include <ciso646>

#include "type_analysis.h"
#include "compiler.h"

using namespace std;
using namespace llvm;

namespace rift {

    char TypeAnalysis::ID = 0;

    void TypeAnalysis::genericArithmetic(CallInst * ci) {
        AType * lhs = state.get(ci->getOperand(0));
        AType * rhs = state.get(ci->getOperand(1));
        state.set(ci, lhs->merge(rhs));
    }

    void TypeAnalysis::genericRelational(CallInst * ci) {
        AType * lhs = state.get(ci->getOperand(0));
        AType * rhs = state.get(ci->getOperand(1));
        if (lhs->isScalar() and rhs->isScalar()) {
            state.set(ci,
                      new AType(AType::Kind::R,
                                AType::Kind::DV,
                                AType::Kind::D));
        } else {
            state.set(ci, new AType(AType::Kind::R, AType::Kind::DV));
        }
    }


    void TypeAnalysis::genericGetElement(CallInst * ci) {
        AType * from = state.get(ci->getOperand(0));
        AType * index = state.get(ci->getOperand(1));
        if (from->isDouble()) {
            if (index->isScalar()) {
                state.set(ci,
                            new AType(AType::Kind::R,
                                      AType::Kind::DV,
                                      AType::Kind::D));
            } else {
                state.set(ci, new AType(AType::Kind::R, AType::Kind::DV));
            }
        } else if (from->isCharacter()) {
            state.set(ci, new AType(AType::Kind::R, AType::Kind::CV));
        } else {
            state.set(ci, new AType(AType::Kind::T));
        }
    }

    bool TypeAnalysis::runOnFunction(llvm::Function & f) {
        // std::cout << "runnning TypeAnalysis..." << std::endl;
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
                            state.set(op, new AType(AType::Kind::D));
                            state.set(ci, new AType(AType::Kind::DV));
                        } else if (s == "characterVectorLiteral") {
                            state.set(ci, new AType(AType::Kind::CV));
                        } else if (s == "fromDoubleVector") {
                            state.set(ci,
                                    new AType(AType::Kind::R,
                                        state.get(ci->getOperand(0))));
                        } else if (s == "fromCharacterVector") {
                            state.set(ci,
                                    new AType(AType::Kind::R,
                                        state.get(ci->getOperand(0))));
                        } else if (s == "fromFunction") {
                            state.set(ci,
                                    new AType(AType::Kind::R,
                                        state.get(ci->getOperand(0))));
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
                            state.set(ci, new AType(AType::Kind::D));
                        } else if (s == "type") {
                            // result of type operation is always 
                            // character vector
                            state.set(ci, new AType(AType::Kind::CV));
                        } else if (s == "c") {
                            // make sure the types to c are correct
                            AType * t1 = state.get(ci->getArgOperand(1));
                            for (unsigned i = 2; i < ci->getNumArgOperands(); ++i)
                                t1 = t1->merge(state.get(ci->getArgOperand(i)));
                            if (t1->isScalar())
                                // concatenation of scalars is a vector
                                t1 = new AType(AType::Kind::R, AType::Kind::DV);
                            state.set(ci, t1);
                        } else if (s == "genericEval") {
                            state.set(ci, new AType(AType::Kind::R));
                        } else if (s == "envGet") {
                            state.set(ci, new AType(AType::Kind::R));
                        } else if (s == "envSet") {
                            state.set(ci->getOperand(1),
                                    state.get(ci->getOperand(2)));
                        }
                    } else if (PHINode * phi = dyn_cast<PHINode>(&i)) {
                        AType * first = state.get(phi->getOperand(0));
                        AType * second = state.get(phi->getOperand(1));
                        AType * result = first->merge(second);
                        state.set(phi, result);
                    }
                }
            }
        } while (!state.hasReachedFixpoint());
        // f.dump();
        // cout << state << endl;
        return false;
    }

    std::ostream & operator << (std::ostream & s, MachineState const & m) {
        s << "Abstract State: " << "\n";
        for (auto const & v : m.type) {
            auto location = std::get<0>(v);
            auto state = std::get<1>(v);
            if (location) {
                llvm::raw_os_ostream ss(s);
                location->printAsOperand(ss, false);
                ss.flush();
                s << " ";
            }
            s << *state << endl;
        }
        return s;
    }

    std::ostream & operator << (std::ostream & s, AType const & t) {
        llvm::raw_os_ostream ss(s);
        switch (t.kind) {
            case AType::Kind::D:
                ss << " D ";
                break;
            case AType::Kind::DV:
                ss << " DV ";
                break;
            case AType::Kind::CV:
                ss << " CV ";
                break;
            case AType::Kind::F:
                ss << " F ";
                break;
            case AType::Kind::R:
                ss << " R ";
                break; 
            case AType::Kind::T:
                ss << " T ";
                break;
	   case AType::Kind::B:
                ss << " B ";
                break;
       }
        ss.flush();
        if (t.payload != nullptr)
            s << " -> " << *(t.payload);
        return s;
    }

} // namespace rift


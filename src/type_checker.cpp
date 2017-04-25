#include <ciso646>
#include "type_checker.h"

using namespace llvm;

namespace rift {

char TypeChecker::ID = 0;

bool TypeChecker::runOnFunction(llvm::Function & f) {
    state.clear();
    do {
        state.iterationStart();
        for (auto & b : f) {
            for (auto & i : b) {
                if (CallInst * ci = dyn_cast<CallInst>(&i)) {
                    StringRef s = ci->getCalledFunction()->getName();
                    if (s == "doubleVectorLiteral") {
                        state.update(ci, Type::D);
                    } else if (s == "fromDoubleVector") {
                        state.update(ci, Type::D);
                    } else if (s == "characterVectorLiteral") {
                        state.update(ci, Type::C);
                    } else if (s == "fromCharacterVector") {
                        state.update(ci, Type::C);
                    } else if (s == "genericSub") {
                        Type t1 = state.get(ci->getOperand(0));
                        Type t2 = state.get(ci->getOperand(1));
                        switch (t1) {
                            case Type::D:
                            case Type::T:
                                switch (t2) {
                                    case Type::D:
                                    case Type::T:
                                        state.update(ci, Type::D);
                                        break;
                                    default:
                                        throw "TypeErr: Invalid types for binary subtraction";
                                }
                                break;
                            default:
                                throw "TypeErr: Invalid types for binary subtraction";
                        }
                    } else if (s == "fromFunction") {
                        state.update(ci, Type::F);
                    } else if (s == "genericGetElement") {
                        Type vector_t = state.get(ci->getOperand(0));
                        Type index_t  = state.get(ci->getOperand(1));
                        if (vector_t == Type::F) {
                            throw "TypeErr: Cannot index functions";
                        }
                        switch (index_t) {
                            case Type::C:
                                throw "TypeErr: character is not a valid index";
                            case Type::F:
                                throw "TypeErr: A function is not a valid index";
                            case Type::T:
                            case Type::D:
                                state.update(ci, vector_t);
                                break;
                            default:
                                assert(false);
                        }

                    //Complete me

                    } else {
                        // If we still don't know nothing about the instruction
                        // set it to T, since it could be anything. If we handle
                        // all cases above this case should not be reached
                        // anymore.
                        if (state.get(ci) == Type::B) {
                            state.update(ci, Type::T);
                        }
                    }
                }
            }
        }
    } while ( not state.hasReachedFixpoint());
    return false;
}

}

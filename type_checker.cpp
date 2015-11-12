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
                            state.upddate(ci, Type::D);
                        } else if (s == "fromDoubleVector") {
                            state.upddate(ci, Type::D);
                        } else if (s == "characterVectorLiteral") {
                            state.upddate(ci, Type::C);
                        } else if (s == "fromCharacterVector") {
                            state.upddate(ci, Type::C);
                        } else if (s == "genericSub") {
                            Type t1 = state.get(ci->getOperand(0));
                            Type t2 = state.get(ci->getOperand(1));
                            switch (t1) {
                                case Type::D:
                                case Type::T:
                                    switch (t2) {
                                        case Type::D:
                                        case Type::T:
                                            state.upddate(ci, Type::D);
                                            break;
                                        default:
                                            throw "Invalid types for binary subtraction";
                                    }
                                    break;
                                default:
                                    throw "Invalid types for binary subtraction";
                            }
                        }
                    }
                }
            }
        } while ( not state.hasReachedFixpoint());
        return false;
    }

}

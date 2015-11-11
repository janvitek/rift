#include "type_checker.h"

using namespace llvm;

namespace rift {

    char TypeChecker::ID = 0;

    bool TypeChecker::runOnFunction(llvm::Function & f) {
        types_.clear();
        do {
            changed = false;
            for (auto & b : f) {
                for (auto & i : b) {
                    if (CallInst * ci = dyn_cast<CallInst>(&i)) {
                        StringRef s = ci->getCalledFunction()->getName();
                        if (s == "doubleVectorLiteral") {
                            setValueType(ci, Type::D);
                        } else if (s == "fromDoubleVector") {
                            setValueType(ci, Type::D);
                        } else if (s == "characterVectorLiteral") {
                            setValueType(ci, Type::C);
                        } else if (s == "fromCharacterVector") {
                            setValueType(ci, Type::C);
                        } else if (s == "genericSub") {
                            Type t1 = valueType(ci->getOperand(0));
                            Type t2 = valueType(ci->getOperand(1));
                            switch (t1) {
                                case Type::D:
                                case Type::T:
                                    switch (t2) {
                                        case Type::D:
                                        case Type::T:
                                            setValueType(ci, Type::D);
                                            break;
                                        default:
                                            throw "Invalid types for binary subtraction";
                                    }
                                    break;
                                default:
                                    throw "Invalid types for binary subtraction";
                            }
                        }
                        //  fill me in
                 // } else if (PHINode * phi = dyn_cast<PHINode>(&i)) {
                        // fill me in
                    }
                }
            }
        } while (changed);
        return false;
    }

}

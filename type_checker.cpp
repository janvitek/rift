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
                        } else if (s == "genericSub") {
                            Type t1 = valueType(ci->getOperand(0));
                            Type t2 = valueType(ci->getOperand(1));
                            if (t1 != t2)
                                throw "Types into binary minus must be the same";
                            if (t1 != Type::D)
                                throw "Only doubles can be subtracted";
                            setValueType(ci, Type::D);
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

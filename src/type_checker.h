#pragma once
#ifndef TYPE_CHECKER_H
#define TYPE_CHECKER_H

#include "llvm.h"
#include "abstract_state.h"

namespace rift {


/** The point of the typechecker is to check that the program conforms to the very basic set of rules. These are by far not exhaustive, but they will catch at least something.

  The only types you should worry about are D for double vectors, C for character vectors and F for functions. T is the top element of the lattice, meaning that the type of the RVal is uknown.

  There is some example code in the implementation of runOnFunction() method, we have prefilled the fromDoubleVector, doubleVectorLiteral and genericSub runtime calls. Your task is to extend this functionality further, including the implementation of phi nodes, where two values merge into one. For this, you will need to fill in proper ordering in the method lessThan.


*/


class TypeChecker : public llvm::FunctionPass {
public:

    enum Type {
        B,
        D,
        C,
        F,
        T,
    };

    typedef AbstractState<Type> State;

    static char ID;

    llvm::StringRef getPassName() const override {
        return "TypeChecker";
    }

    TypeChecker() : llvm::FunctionPass(ID) {}

    bool runOnFunction(llvm::Function & f) override;

private:
    State state;
};


inline bool operator < (TypeChecker::Type t1, TypeChecker::Type t2) {
    if (t1 == TypeChecker::Type::T) return false;
    if (t1 == TypeChecker::Type::B) return !t2 == TypeChecker::Type::B;

    if (t2 == TypeChecker::Type::B) return false;
    if (t2 == TypeChecker::Type::T) return true;

    if (t1 == t2) return false;

    assert(false);
}


} // namespace rift



#endif // TYPE_CHECKER_H

#pragma once
#ifndef TYPE_CHECKER_H
#define TYPE_CHECKER_H

#include "llvm.h"

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



    class MachineState {
    public:
        Type get(llvm::Value * v) {
            if (type.count(v))
                return type.at(v);
            else
                return Type::B;
        }

        void upddate(llvm::Value * v, Type t) {
            Type prev = get(v);
            if (prev < t) {
                type[v] = t;
                changed = true;
            }
        }

        void clear() {
            type.clear();
        }

        void iterationStart() {
            changed = false;
        }

        bool hasReachedFixpoint() {
            return !changed;
        }

        MachineState() : changed(false) {}

        friend std::ostream & operator << (std::ostream & s, MachineState & m);

    private:

        bool changed;

        std::map<llvm::Value *, Type> type;

    };



    static char ID;

    char const * getPassName() const override {
        return "TypeChecker";
    }

    TypeChecker() : llvm::FunctionPass(ID) {}

    bool runOnFunction(llvm::Function & f);

private:
    MachineState state;

};




inline bool operator < (TypeChecker::Type t1, TypeChecker::Type t2) {
    // fill me in
    return false;
}


} // namespace rift





#endif // TYPE_CHECKER_H

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

        static char ID;

        char const * getPassName() const override {
            return "TypeChecker";
        }

        TypeChecker() : llvm::FunctionPass(ID) {}

        bool runOnFunction(llvm::Function & f);
    private:
        enum class Type {
            D,
            C,
            F,
            T
        };

        Type valueType(llvm::Value * v) {
            auto i = types_.find(v);
            if (i == types_.end())
                return Type::T;
            else return i->second;
        }

        void setValueType(llvm::Value * v, Type t) {
            auto i = types_.find(v);
            if (i == types_.end()) {
                changed = true;
                types_[v] = t;
            } else {
                if (lessThan(i->second, t)) {
                    changed = true;
                    i->second = t;
                }
            }

        }

        bool lessThan(Type t1, Type t2) {
            // correct me!
            return t1 != t2;
        }


        std::map<llvm::Value *, Type> types_;

        bool changed;

    };




} // namespace rift





#endif // TYPE_CHECKER_H
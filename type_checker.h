#pragma once
#ifndef TYPE_CHECKER_H
#define TYPE_CHECKER_H

#include "llvm.h"

namespace rift {

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
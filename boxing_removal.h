#pragma once
#ifndef BOXING_REMOVAL_H
#define BOXING_REMOVAL_H

#include "llvm.h"
#include "unboxing.h"

namespace rift {

    class BoxingRemoval : public llvm::FunctionPass {
    public:
        static char ID;
        BoxingRemoval() : llvm::FunctionPass(ID) {}

        char const * getPassName() const override {
            return "BoxingRemoval";
        }

        void getAnalysisUsage(llvm::AnalysisUsage & AU) const override {
            AU.addRequired<Unboxing>();
        }

        bool runOnFunction(llvm::Function & f) override;
    };




} // namespace rift

#endif // BOXING_REMOVAL_H
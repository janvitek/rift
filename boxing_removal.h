#pragma once
#ifndef BOXING_REMOVAL_H
#define BOXING_REMOVAL_H

#include "llvm.h"
#include "unboxing.h"


namespace rift {

    /** Boxing removal optimization deletes all pure (readnone in llvm's terminology) functions. As the unboxing pass makes many boxing statements dead and creates more boxing statements which might be dead, this pass is essential in removing this code. */
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
#pragma once
#ifndef UNBOXING_H
#define UNBOXING_H

#include "llvm.h"
#include "type_analysis.h"

namespace rift {

    class RiftModule;

    class Unboxing : public llvm::FunctionPass {
    public:
        static char ID;

        Unboxing() : llvm::FunctionPass(ID) {}

        char const * getPassName() const override {
            return "Unboxing";
        }

        void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
            AU.addRequired<TypeAnalysis>();
        }

        bool runOnFunction(llvm::Function & f) override;

    protected:

        llvm::Value * unbox(llvm::CallInst * ci, llvm::Value * v);

        llvm::Value * boxDoubleScalar(llvm::CallInst * ci, llvm::Value * scalar);

        llvm::Value * boxDoubleVector(llvm::CallInst * ci, llvm::Value * vector);
        llvm::Value * boxCharacterVector(llvm::CallInst * ci, llvm::Value * vector);

        bool doubleArithmetic(llvm::CallInst * ci, llvm::Instruction::BinaryOps op, llvm::Function * fop);

        bool doubleComparison(llvm::CallInst * ci, llvm::CmpInst::Predicate op, llvm::Function * fop);

        bool characterOperator(llvm::CallInst * ci, llvm::Function * fop);

        bool genericAdd(llvm::CallInst * ci);
        bool genericEq(llvm::CallInst * ci);
        bool genericNeq(llvm::CallInst * ci);

        bool genericGetElement(llvm::CallInst * ci);
        bool genericSetElement(llvm::CallInst * ci);
        bool genericC(llvm::CallInst * ci);

        RiftModule * m;
        TypeAnalysis * ta;

    };



} // namespace rift




#endif // UNBOXING_H



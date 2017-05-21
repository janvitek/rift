#if VERSION > 10

#pragma once

#include "llvm.h"
#include "type_analysis.h"

namespace rift {

class RiftModule;

/** Given the type analysis, replaces generic calls with their type-specific
    variants where possible. */
class Specialize : public llvm::FunctionPass {
public:
    static char ID;
    Specialize() : llvm::FunctionPass(ID) {}

    llvm::StringRef getPassName() const override { return "Specialize"; }

    void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
        AU.addRequired<TypeAnalysis>();
        AU.addPreserved<TypeAnalysis>();
    }
    bool runOnFunction(llvm::Function & f) override;

protected:
    void updateDoubleScalar(llvm::Value * newVal);
    void updateDoubleOp(llvm::Function * fun,
                        llvm::Value * arg1, llvm::Value * arg2,
                        AType * res);
    void updateCharOp(llvm::Function * fun,
                      llvm::Value * arg1, llvm::Value * arg2,
                      AType * res);
    void genericAdd();
    void genericArithmetic(llvm::Function * fop);
    void genericRelational(llvm::Function * fop);
    void genericComparison(llvm::Value * lhs, llvm::Value * rhs,
                           AType * lhsType, AType * rhsType,
                           llvm::Function * fop, llvm::Function * cop);
    void genericEq();
    void genericNeq();
    void genericGetElement();
    void genericC();
    void genericEval();

    /** Rift module currently being optimized.  */
    llvm::Module * m;
    /** Type analysis results */
    TypeAnalysis * ta;
    /** Abstract state computed by the type analysis */
    State & state() { return ta->state; }
    /** Current instruction. */
    llvm::Instruction * ins;
    /** True if we changed the code */
    bool changed_;
};
} // namespace rift
#endif //VERSION

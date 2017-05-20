#if VERSION > 10

#pragma once

#include "llvm.h"
#include "type_analysis.h"

namespace rift {

class RiftModule;

/** Given the type & shape analysis, replaces generic calls with their type specific variants where the information is available. 

  As the optimization progresses, the type analysis results are refined and updated with the newly inserted values and unboxing chains. 

  Note that the algorithm does not make use of all possible optimizations. For instance, while unboxed functions are defined in the type analysis, the optimization does not specialize on them yet. 
  */
class Specialize : public llvm::FunctionPass {
public:
    static char ID;

    Specialize() : llvm::FunctionPass(ID) {}

    llvm::StringRef getPassName() const override {
        return "Specialize";
    }

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

    /** Rift module currently being optimized, obtained from the function.

      The module is used for the declarations of the runtime functions. 
      */
    llvm::Module * m;

    /** Preexisting type analysis that is queried to obtain the type & shape information as well as the boxing chains. */
    TypeAnalysis * ta;
    State & state() { return ta->state; }

    llvm::Instruction * ins;

    // did the code change during optimization?
    bool changed_;

};

} // namespace rift


#endif //VERSION

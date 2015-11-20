
#pragma once
#ifndef SPECIALIZE_H
#define SPECIALIZE_H

#include "llvm.h"
#include "type_analysis.h"

namespace rift {

class RiftModule;

/** Given the type & shape analysis, replaces generic calls with their type specific variants where the information is available. 

  As the optimization progresses, the type analysis results are refined and updated with the newly inserted values and unoxing chains. 

  Note that the algorithm does not make use of all possible optimizations. For instance, while unboxed functions are defined in the type analysis, the optimization does not specialize on them yet. 
  */
class Specialize : public llvm::FunctionPass {
public:
    static char ID;

    Specialize() : llvm::FunctionPass(ID) {}

    char const * getPassName() const override {
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

    bool genericAdd();

    bool genericArithmetic(llvm::Function * fop);

    bool genericRelational(llvm::Function * fop);

    bool genericComparison(llvm::Value * lhs, llvm::Value * rhs,
                           AType * lhsType, AType * rhsType,
                           llvm::Function * fop, llvm::Function * cop);

    bool genericEq();

    bool genericNeq();

    bool genericGetElement();

    bool genericC();

    bool genericEval();

    /** Rift module currently being optimized, obtained from the function.

      The module is used for the declarations of the runtime functions. 
      */
    RiftModule * m;

    /** Preexisting type analysis that is queried to obtain the type & shape information as well as the boxing chains. */
    TypeAnalysis * ta;
    AbstractState & state() { return ta->state; }

    llvm::Instruction * ins;

};



} // namespace rift




#endif // UNBOXING_H



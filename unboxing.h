
#pragma once
#ifndef UNBOXING_H
#define UNBOXING_H

#include "llvm.h"
#include "type_analysis.h"

namespace rift {

class RiftModule;

/** Given the type & shape analysis, replaces generic calls with their type specific variants where the information is available. 

  As the optimization progresses, the type analysis results are refined and updated with the newly inserted values and unoxing chains. 

  Note that the algorithm does not make use of all possible optimizations. For instance, while unboxed functions are defined in the type analysis, the optimization does not specialize on them yet. 
  */
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

    AType * updateAnalysis(llvm::Value * value, AType * type);

    llvm::Value * box(AType * what);

    llvm::Value * unbox(AType * t);

    llvm::Value * getScalarPayload(AType * t);

    llvm::Value * getVectorPayload(AType * t);

    void doubleArithmetic(AType * lhs, AType * rhs, llvm::Instruction::BinaryOps op, llvm::Function * fop);

    bool genericAdd();

    bool genericArithmetic(llvm::Instruction::BinaryOps op, llvm::Function * fop);

    void doubleRelational(AType * lhs, AType * rhs, llvm::CmpInst::Predicate op, llvm::Function * fop);

    bool genericRelational(llvm::CmpInst::Predicate op, llvm::Function * fop);

    bool genericComparison(AType * lhs, AType * rhs, llvm::CmpInst::Predicate op, llvm::Function * fop, llvm::Function * cop);

    bool genericEq();

    bool genericNeq();

    bool genericGetElement();

    bool genericSetElement();

    bool genericC();

    bool genericEval();

    /** Rift module currently being optimized, obtained from the function.

      The module is used for the declarations of the runtime functions. 
      */
    RiftModule * m;

    /** Preexisting type analysis that is queried to obtain the type & shape information as well as the boxing chains. */
    TypeAnalysis * ta;
    MachineState & state() { return ta->state; }

    llvm::Instruction * ins;

};



} // namespace rift




#endif // UNBOXING_H



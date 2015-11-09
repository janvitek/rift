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

        /** Given a boxed value, returns a value that is one step unboxed. If such value does not exist, inserts a call to unboxing runtime function and associates the two values for future reference. 
         */
        llvm::Value * unbox(llvm::CallInst * ci, llvm::Value * v);

        /** Given DoubleScalar value (represented as double in the ir), creates its boxed variants. 
        
        The method creates both the double vector containing the scalar and the value boxing the double vector with it. 

        These are necessary for correct function of the algorithm and if not used, they will be deleted by the boxing removal pass. 
         */
        llvm::Value * boxDoubleScalar(llvm::CallInst * ci, llvm::Value * scalar);

        /** Given DoubleVector value, creates its boxed variant. 
        
        The boxed value is necessary for correct function of the algorithm and if not used, they it will be deleted by the boxing removal pass.
        */
        llvm::Value * boxDoubleVector(llvm::CallInst * ci, llvm::Value * vector);

        /** Given CharacterVector value, creates its boxed variant.

        The boxed value is necessary for correct function of the algorithm and if not used, they it will be deleted by the boxing removal pass.
        */
        llvm::Value * boxCharacterVector(llvm::CallInst * ci, llvm::Value * vector);

        /** Arithmetic operator on double vectors. 
        
        Checks that the type & shape of the incomming vectors allows for optimization, and if so replaces the generic call with specified runtime function, or machine instruction for double scalars, creates its boxed variants and replaces all uses of the old result with the new one. 
        */
        bool doubleArithmetic(llvm::CallInst * ci, llvm::Instruction::BinaryOps op, llvm::Function * fop);

        /** Comparison on double vectors. 

        Checks that the type & shape of the incomming vectors allows for optimization, and if so replaces the generic call with specified runtime function, or machine instruction for double scalars, creates its boxed variants and replaces all uses of the old result with the new one.
        */
        bool doubleComparison(llvm::CallInst * ci, llvm::CmpInst::Predicate op, llvm::Function * fop);

        /** Operator on two character vectors. 
        
        If the result is CharacterVector, it is also boxed into Value which replaces all usages of the previous result. 

        Currently we only use this for character addition.
        */
        bool characterArithmetic(llvm::CallInst * ci, llvm::Function * fop);
        
        /** Comparison of character vectors. 

        The result of the comparison is always a double vector. 
        */
        bool characterComparison(llvm::CallInst * ci, llvm::Function * fop);

        /** Optimization of generic add intrinsic. 
        
        Based on the type & shape of the incomming values, leaves the generic call or optimizes it to a scalar operation, double vector, or double character call. 
        */
        bool genericAdd(llvm::CallInst * ci);
        
        /** Generic equality test optimization. 
        
        Based on incomming types & shapes, calls special intrinsics, or fills in the constant value when comparing two values of unrelated types.

        */
        bool genericEq(llvm::CallInst * ci);
        
        /** Generic inequality test optimization.

        Based on incomming types & shapes, calls special intrinsics, or fills in the constant value when comparing two values of unrelated types.

        */
        bool genericNeq(llvm::CallInst * ci);

        /** Optimizes the getElement call based on incomming types and shapes so that double or character variants are called and in double's case, if the index vector is scalar, a double scalar is returned. */
        bool genericGetElement(llvm::CallInst * ci);

        /** Generic set element is optimized to double scalar, double and character possibilities. 
         */
        bool genericSetElement(llvm::CallInst * ci);

        /** Concatenation is optimized to either character only, or double only variant, if possible. 
         */
        bool genericC(llvm::CallInst * ci);

        /** Rift module currently being optimized, obtained from the function.
        
        The module is used for the declarations of the runtime functions. 
        */
        RiftModule * m;

        /** Preexisting type analysis that is queried to obtain the type & shape information as well as the boxing chains. */
        TypeAnalysis * ta;

    };



} // namespace rift




#endif // UNBOXING_H



#pragma once
#ifndef TYPE_ANALYSIS_H
#define TYPE_ANALYSIS_H

#include "llvm.h"
#include "abstract_state.h"

/** Abstract interpretation-based analysis of Rift programs. The analysis
  operates over the LLVM IR and the rift value types
  The definitions in this file add up to an intra-procedural, flow sensitive
  analysis of Rift functions.  The abstract state consists of mapping from LLVM
  values (which are registers containing RVals) to abstract types.
  */
namespace rift {

/** An abstract type, or AType, represents an object in the heap of a Rift
  program. AType forms the following lattice, where D1 is a DoubleVector of
  size 1, DV a DoubleVector, CV a CharacterVector, F an RFun, and T any kind
  of RVal.

                            T

                        /   |    \
                            |     \
                    DV      |      \

                    |       CV      F

                    D1      |     /
                            |    /
                       \    |   /

                            B
          
  */
class AType {

public:
    //Singleton Type instances
    static AType * T;  // Top (Rval)
    static AType * B;  // Bottom
    static AType * D1; // Double Vector of length 1
    static AType * DV; // Double Vector
    static AType * CV; // Character Vector
    static AType * F;  // Function

    //////////////////// Abstract Operations //////////////////

    /** Returns a new AType that is the least upper bound of the this
      and the argument.
      */
    AType * merge(AType * a) {
        if (a == T)        return T;
        if (this == B)     return a;
        if (a == B)        return this;
    
        if (this == D1) {
            if (a == D1)  return a;
            if (a == DV) return a;
            return T;
        }

        if ((this == DV || this == CV || this == F) &&
             this == a) {
            return this;
        }

        return T;
    }

    /** Is this the top element of the lattice. */
    bool isTop() const {
        return this == T;
    }

    bool isBottom() const {
        return this == B;
    }

    /** Is this a scalar value? */
    bool isDoubleScalar() const {
        return this == D1;
    }

    /** Is this a value that can be unboxed to a double? */
    bool isDouble() const {
        return this == D1 || this == DV;
    }

    /** Is this a string? */
    bool isCharacter() const {
        return this == CV;
    }

    /** Two ATypes are similar if they have the same kind */
    bool isSimilar(AType * other) const {
        return (isCharacter() && other->isCharacter()) ||
               (isDouble() && other->isDouble());
    }

    /** Order on ATypes. 
      */
    bool operator < (AType const & other) const {
        if(isTop()) return false;
        if(other.isTop()) return true;
        if(isBottom()) return !other.isBottom();
        
        if (isDoubleScalar()) return !other.isDoubleScalar();
        
        // The only valid options left
        assert((this == DV && &other == D1) || this == &other);

        return false;
    }

private:
    friend std::ostream & operator << (std::ostream & s, AType & m);

    AType(const std::string name) : name(name) {}
    const std::string name;
};

/** Type analysis.  */
class TypeAnalysis : public llvm::FunctionPass {
public:
    typedef AbstractState<AType*, llvm::Value*> State;
    static char ID;

    llvm::StringRef getPassName() const override { return "TypeAnalysis"; }

    TypeAnalysis() : llvm::FunctionPass(ID) {}

    bool runOnFunction(llvm::Function & f) override;

    State state;

private:
    void genericArithmetic(llvm::CallInst * ci);
    void genericRelational(llvm::CallInst * ci);
    void genericGetElement(llvm::CallInst * ci);
};

} // namespace rift

#endif // TYPE_ANALYSIS_H

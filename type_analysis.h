#pragma once
#ifndef TYPE_ANALYSIS_H
#define TYPE_ANALYSIS_H

#include "llvm.h"

/** Abstract interpretation-based analysis of Rift programs. The analysis
  operates over the LLVM IR and the data types that appear at that
  level. This means all types exposed in the implementation are visible,
  including raw doubles and going all the way to RVals. The definitions in
  this file add up to an intra-procedural, flow sensitive analysis of Rift
  functions.  The abstract state consists of mapping from LLVM values
  (which are registers and constants) to abstract types. There are also
  some free floating abstract types that are not referenced by registers.
  */
namespace rift {
// forward decl..
class MachineState;

/** An abstract type, or AType, represents a portion of the heap of a Rift
  program. Each AType has a kind -- this describes the type of value we
  are dealine with a kind can be either one of B(ottom), T(op), D(ouble),
  D(ouble)V(ector) and others.
  */
class AType {

public:
    //Singleton Type instances
    static AType * T;  // Top
    static AType * B;  // Bottom
    static AType * D;  // Double Vector of length 1
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
    
        if (this == D) {
            if (a == D)  return a;
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
        return this == D;
    }

    /** Is this a value that can be unboxed to a double? */
    bool isDouble() const {
        return this == D || this == DV;
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
        assert(this == &other);
        return false;
    }

    void print(std::ostream & s, MachineState & m);

private:
    AType() {}
};

/**
  The MachineState represents the abstact state of the function being
  analyzed modulo unnamed ATypes. It's role is to maintain a mapping
  between LLVM values and the abstract information computed by the 
  analysis.
  */
class MachineState {
public:
    AType * get(llvm::Value * v) {
        if (type.count(v)) return type.at(v);
        auto n = AType::B;
        type[v] = n;
        return n;
    }
    
    llvm::Value * getScalarLocation(llvm::Value * v) {
        if (!scalarLocation.count(v)) return nullptr;
        return scalarLocation.at(v);
    }

    AType * initialize(llvm::Value * v, AType * t) {
        type[v] = t;
        return t;
    }

    // If we know where the scalar value comes from we store it for later
    // unboxing by the optimization pass.
    AType * update(llvm::Value * v, AType * t, llvm::Value * sl) {
        assert(scalarLocation.count(v) == 0 || scalarLocation.at(v) == sl);
        assert(t->isDoubleScalar());
        scalarLocation[v] = sl;
        return update(v, t);
    }

    AType * update(llvm::Value * v, AType * t) {
        auto prev = get(v);
        if (*prev < *t) {
            type[v] = t;
            changed = true;
            return t;
        }
        return prev;
    }

    void clear() {
        type.clear();
        scalarLocation.clear();
    }
     
    void erase(llvm::Value * v) {
        scalarLocation.erase(v);
        type.erase(v);
    }

    void iterationStart() {
        changed = false;
    }

    bool hasReachedFixpoint() {
        return !changed;
    }

    MachineState() : changed(false) {}

    friend std::ostream & operator << (std::ostream & s, MachineState & m);

private:
    bool changed;
    std::map<llvm::Value*, AType*> type;
    std::map<llvm::Value*, llvm::Value*> scalarLocation;
};

/** Type and shape analysis.  */
class TypeAnalysis : public llvm::FunctionPass {
public:
    static char ID;

    char const * getPassName() const override { return "TypeAnalysis"; }

    TypeAnalysis() : llvm::FunctionPass(ID) {}

    bool runOnFunction(llvm::Function & f) override;

    MachineState state;

private:
    void genericArithmetic(llvm::CallInst * ci);
    void genericRelational(llvm::CallInst * ci);
    void genericGetElement(llvm::CallInst * ci);
};

} // namespace rift

#endif // TYPE_ANALYSIS_H

#if VERSION > 10
#pragma once

#include <set>
#include <sstream>
#include <memory>
#include "llvm.h"
#include "rift.h"
/** Abstract interpretation-based analysis of Rift. The analysis
  operates over the LLVM IR and the rift value types. This is an 
  intra-procedural, flow sensitive analysis. The abstract state 
  consists of mapping from LLVM values (which are registers containing
  RVals) to abstract types. */
namespace rift {
/** An abstract type, or AType, represents an object in the heap. ATypes
  form a lattice. D1 is a DoubleVector of len 1, DV a DoubleVector, 
  CV a CharacterVector, F an RFun, and T any kind of RVal.
                            T
                        /   |    \
                      /     |     \
                    DV      |      \
                    |       CV      F
                    D1      |     /
                      \     |    /
                       \    |   /
                            B
  */
class AType {

public:
    //Singletons
    static AType * T;  // Top
    static AType * D1; // Double Vector of length 1
    static AType * DV; // Double Vector
    static AType * CV; // Character Vector
    static AType * F;  // Function
    static AType * B;  // Bottom

    //////////////////// Abstract Operations //////////////////

    /** Returns the AType that is the least upper bound of this and a.*/
    AType * merge(AType  *  a)  {
        if (this == a) return this;

        if (a == T || this == T) return T;
        if (this == B) return a;
        if (a == B) return this;

        if (this == D1 and a == DV) return DV;
        if (this == DV and a == D1) return DV;

        return T;
    }

    /** Is it the top of the lattice? */
    bool isTop() const { return this == T; }

    /** Is it the bottom of the lattice? */
    bool isBottom() const { return this == B; }

    /** Is this a scalar numeric value? */
    bool isDoubleScalar() const { return this == D1; }

    /** Is this a double? */
    bool isDouble() const { return this == D1 || this == DV; }

    /** Is this a string? */
    bool isCharacter() const { return this == CV; }

    /** Is this a function? */
    bool isFun() const { return this == F; }
    
    /** Two ATypes are similar if they have the same kind */
    bool isSimilar(AType * other) const {
        return (isCharacter() && other->isCharacter()) ||
               (isDouble() && other->isDouble()) ||
               (isFun() && other->isFun());
    }

    /** Order, < returns true if this is strictly smaller than other. */
    bool operator < (AType * other)  {
        AType* m = this->merge(other);
        return  m == other;
    }

private:
    friend ostream & operator << (ostream & s, AType & m);
    AType(const string name) : name(name) {}
    const string name;
};

/**
 The AbstractState represents the abstact state of the function being
 analyzed. It's role is to maintain a mapping between LLVM values and the
 abstract information computed by the analysis.
 Analysis can attach metadata to LLVM values for later use. The metadata
 is not part of the abstract state, but merely associated information for
 later use, e.g. for the optimization.
 */
class State {

public:
    AType* get(llvm::Value * v) {
        if (type.count(v)) return type.at(v);
        changed = true;
        auto n = AType::B;
        type[v] = n;
        return n;
    }

    AType* initialize(llvm::Value * v, AType* t) {
        type[v] = t;
        return t;
    }

    AType* update(llvm::Value * v, AType* t) {
        auto prev = get(v);
        if (prev < t) {
            type[v] = t;
            changed = true;
            return t;
        }
        return prev;
    }

    void clear() {
        type.clear();
    }

    void erase(llvm::Value * v) {
        type.erase(v);
    }

    void iterationStart() {
        changed = false;
    }

    bool hasReachedFixpoint() {
        return !changed;
    }

    State() : changed(false) {}

    void print(ostream & s) {
        s << "Abstract State: " << "\n";
        struct cmpByName {
            bool operator()(const llvm::Value * a, const llvm::Value * b) const {
                int aName, bName;
                stringstream s;
                llvm::raw_os_ostream ss(s);
                a->printAsOperand(ss, false);
                ss.flush();
                s.seekg(1);
                s >> aName;
                stringstream s2;
                llvm::raw_os_ostream ss2(s2);
                b->printAsOperand(ss2, false);
                ss2.flush();
                s2.seekg(1);
                s2 >> bName;
                return aName < bName;
            }
        };
        
        set<llvm::Value*, cmpByName> sorted;
        for (auto const & v : type) {
            auto pos = v.first;
            sorted.insert(pos);
        }
        
        for (auto pos : sorted) {
            auto st = type.at(pos);
            llvm::raw_os_ostream ss(s);
            pos->printAsOperand(ss, false);
            ss.flush();
            s << *st;
            s << endl;
        }
    }
    
private:
    bool changed;
    map<llvm::Value*, AType*> type;
};


/** Type analysis.  */
class TypeAnalysis : public llvm::FunctionPass {
public:
    static char ID;
    State state;

    llvm::StringRef getPassName() const override { return "TypeAnalysis"; }
    TypeAnalysis() : llvm::FunctionPass(ID) {}
    bool runOnFunction(llvm::Function & f) override;

private:
    void genericArithmetic(llvm::CallInst * ci);
    void genericRelational(llvm::CallInst * ci);
    void genericGetElement(llvm::CallInst * ci);
};
} // namespace rift

#endif //VERSION

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
  D(ouble)V(ector), R(val) and others. ATypes that rerpresent boxes also
  have a payload, another AType, that they refer to.
  */
class AType {

public:
    enum class Kind { B, D, DV, CV,  F, R, T } kind;
    AType * payload;


    static AType * top;

    /////// Constructors////////////////////////////
    AType(Kind t, AType * payload) : kind(t), payload(payload) { }

    AType(Kind t) : kind(t), payload(top) { }

    AType(Kind t, Kind t2) : kind(t), payload(new AType(t2)) { }

    AType(Kind t, Kind t2, Kind t3) : kind(t), payload(new AType(t2, t3)) {  }

    AType(AType const & other) = default;


    //////////////////// Abstract Operations //////////////////

    /** Returns a new AType that is the least upper bound of the this
      and the argument.
      */
    AType * merge(AType * a) {
        if (a->isTop())            return new AType(Kind::T);
        if (kind == Kind::B)       return new AType(*a);
        if (a->kind == Kind::B)    return new AType(*this);
        switch (kind) {
            case Kind::D:
                if (*a != Kind::D)  return new AType(Kind::T);
                else                return new AType(Kind::D);
            case Kind::CV:
                if (*a != Kind::CV) return new AType(Kind::T);
                else                return new AType(Kind::CV);
            case Kind::F:
                if (*a != Kind::F)  return new AType(Kind::T);
                else                return new AType(Kind::F);
            case Kind::DV: 
                if (*a != Kind::DV) return new AType(Kind::T);
                else                return new AType(Kind::DV, payload->merge(a->payload));
            case Kind::R:
                if (*a != Kind::R) return new AType(Kind::T);
                else               return new AType(Kind::R, payload->merge(a->payload));
            default:              return new AType(Kind::T);
        }
    }

    /** Is this the top element of the lattice. */
    bool isTop() const {
        return kind == Kind::T;
    }

    /** Is this a scalar value? */
    bool isScalar() {
        return kind == Kind::D or (!isTop() and payload->isScalar());
    }

    /** Is this a value that can be unboxed to a double? */
    bool isDouble() {
        return kind == Kind::D or kind == Kind::DV or (!isTop() and payload->isDouble());
    }

    /** Is this a string? */
    bool isCharacter() {
        return kind == Kind::CV or (!isTop() and payload->isCharacter());
    }

    //TODO: IS this correct ??  Payload?
    bool operator == (AType::Kind other) const {
        return kind == other;
    }

    //TODO: IS this correct ??  Payload?
    bool operator != (AType::Kind other) const {
        return kind != other;
    }

    bool canBeSameTypeAs(AType * other) {
        if (kind != other->kind)
            return false;
        // they are the same kind, the only way then can be different is if their kind is R and their payload is of different kind, otherwise we cannot be sure
        if (kind == Kind::R)
            return payload->canBeSameTypeAs(other->payload);
        return true;

    }

    /** Order on ATypes.  This only implements the part of the relation
      that can happen and that we care about. (Lazy and risky coding)
      We are only interested in the R types as phi nodes are typed.
      R(DV(D))< R(DV)<R<T, R(CV)<R<T, R(F)<R<T
      */
    bool operator < (AType const & other) const {
        if( isTop() ) return false;
        if( other.isTop() ) return true;
        if( kind == Kind::B) return other != Kind::B;
        // This is a bit of a short cut, but it is fine for our use case
        assert(kind == other.kind);
        return *payload < *other.payload;
    }

    void print(std::ostream & s, MachineState & m);

    static AType * createTop() { 
        auto t = new AType(Kind::T, nullptr);
        t->payload = t;
        return t;
    }
};

class MachineState {
public:
    AType * get(llvm::Value * v) {
        if (type.count(v)) {
            return type.at(v);
        }
        auto n = new AType(AType::Kind::B);
        type[v] = n;
        location[n] = v;
        return n;
    }

    void clear() {
        type.clear();
        location.clear();
    }

    llvm::Value * getLocation(AType * t) {
        if (!location.count(t))
            return nullptr;
        return location[t];
    }

    AType * initialize(llvm::Value * v, AType * t) {
        assert(!getLocation(t));
        type[v] = t;
        location[t] = v;
        return t;
    }

    void erase(llvm::Value * v) {
        if (type.count(v))
            location.erase(type.at(v));
        type.erase(v);
    }

    AType * update(llvm::Value * v, AType * t) {
        //    assert(!(llvm::isa<llvm::Constant>(v) && t->payload != nullptr));
        auto prev = get(v);
        if (*prev < *t) {
            type[v] = t;
            location[t] = v;
            changed = true;
            return t;
        }
        return prev;
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
    std::map<AType*, llvm::Value*> location;
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

#pragma once
#ifndef TYPE_ANALYSIS_H
#define TYPE_ANALYSIS_H

#include "llvm.h"

namespace rift {

    class AType {
    public:
        enum class Kind {
            B, // bottom 
            D,
            DV,
            CV, 
            F,
            R,
            T  // top
        } kind;

        AType * payload;

        AType(Kind t, AType * payload) :
            kind(t),
            payload(payload) {
        }

        AType(Kind t) :
            kind(t),
            payload(nullptr) {
        }

        AType(Kind t, Kind t2) :
            kind(t),
            payload(new AType(t2)) {
        }

        AType(Kind t, Kind t2, Kind t3) :
            kind(t),
            payload(new AType(t2, t3)) {
        }

        AType(AType const & other) = default;

        AType * merge(AType * other) {

            if (other == nullptr or other->isTop())
                return new AType(AType::Kind::T);
            if (kind == Kind::B)
                return new AType(*other);
            else if (other->kind == Kind::B)
                return new AType(*this);
            switch (kind) {
                case Kind::D:
                    if (*other != Kind::D)
                        return new AType(AType::Kind::T);
                    else
                        return new AType(Kind::D);
                case Kind::CV:
                    if (*other != Kind::CV)
                        return new AType(AType::Kind::T);
                    else
                        return new AType(Kind::CV);
                case Kind::F:
                    if (*other != Kind::F)
                        return new AType(AType::Kind::T);
                    else
                        return new AType(Kind::F);
                case Kind::DV:
                    if (*other != Kind::DV)
                        return new AType(AType::Kind::T);
                    else
                        return new AType(Kind::DV, payload == nullptr ? nullptr : payload->merge(other->payload));
                case Kind::R:
                    if (*other != Kind::R)
                        return new AType(AType::Kind::T);
                    else
                        return new AType(Kind::R, payload == nullptr ? nullptr : payload->merge(other->payload));
                default:
                    return new AType(AType::Kind::T);
            }
        }

        bool isTop() const {
            return kind == Kind::T;
        }

        bool isScalar() {
            if (kind == Kind::D)
                return true;
            else 
                return payload != nullptr and payload->isScalar();
        }

        bool isDouble() {
            if (kind == Kind::D)
                return true;
            else if (kind == Kind::DV)
                return true;
            else 
                return payload != nullptr and (payload->isDouble());
        }

        bool isCharacter() {
            if (kind == Kind::CV)
                return true;
            else
                return payload != nullptr and (payload->isCharacter());
        }

        bool operator == (AType::Kind other) const {
            return kind == other;
        }

        bool operator != (AType::Kind other) const {
            return kind != other;
        }

        bool canBeSameTypeAs(AType * other) {
            if (kind != other->kind)
                return false;
            // they are the same kind, the only way then can be different is if their kind is R and their payload is of different kind, otherwise we cannot be sure
            if (kind == Kind::R)
                if (payload != nullptr and other->payload != nullptr)
                    return payload->canBeSameTypeAs(other->payload);
            return true;

        }

        bool operator < (AType const & other) const {
            /** We are only interested in the R types as phi nodes are typed.

            R(DV(D)) < R(DV) < R < T
            R(CV) < R < T
            R(F) < R < T
            */
            switch(kind) {
                case Kind::B:
                    return other.kind != Kind::B;
                case Kind::T:
                    if (other.kind != Kind::T) return false;
                    break;
                default:
                    if (other.kind == Kind::T) return true;
                    break;
            }
            assert(kind == other.kind);

            if (payload == nullptr)
                return false;
            if (payload != nullptr and other.payload == nullptr)
                return true;
            else return *payload < *other.payload;
        }

    };

    std::ostream & operator << (std::ostream & s, AType const & t);

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

        llvm::Value * getLocation(AType * t) {
            if (!location.count(t))
                return nullptr;
            return location[t];
        }

        void set(llvm::Value * v, AType * t) {
            auto prev = get(v);
            if (*prev < *t) {
                type[v] = t;
                location[t] = v;
                changed = true;
            }
        }

        void iterationStart() {
            changed = false;
        }

        bool hasReachedFixpoint() {
            return !changed;
        }

        MachineState() : changed(false) {}

        friend std::ostream & operator << (std::ostream & s, MachineState const & m);

    private:
        bool changed;
        std::map<llvm::Value*, AType*> type;
        std::map<AType*, llvm::Value*> location;
    };

    /** Type and shape analysis. 
     
     Based on the calls to the runtime functions, the analysis tracks the following key attributes:

     - for each loc it remembers its shape. 
     - if the value is boxed, and there exists an unboxed version of the same value, it pairs the two together. 

     The unboxing can have one pair (Value -> DoubleVector, Value->CharacterVector, Value->Function), or two pairs(Value->DoubleScalarVector->DoubleScalar).
     */
    class TypeAnalysis : public llvm::FunctionPass {
    public:

        static char ID;

        char const * getPassName() const override {
            return "TypeAnalysis";
        }

        TypeAnalysis() : llvm::FunctionPass(ID) {}

        bool runOnFunction(llvm::Function & f) override;

    private:
        MachineState state;

        void genericArithmetic(llvm::CallInst * ci);
        void genericRelational(llvm::CallInst * ci);
        void genericGetElement(llvm::CallInst * ci);

        bool changed;

    };

} // namespace rift

#endif // TYPE_ANALYSIS_H

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

        llvm::Value * loc;

        AType(Kind t, AType * payload, llvm::Value * loc = nullptr) :
            kind(t),
            payload(payload),
            loc(loc) {
        }

        AType(Kind t, llvm::Value * loc = nullptr) :
            kind(t),
            payload(nullptr),
            loc(loc) {
        }

        AType(Kind t, Kind t2, llvm::Value * loc = nullptr) :
            kind(t),
            payload(new AType(t2)),
            loc(loc) {
        }

        AType(Kind t, Kind t2, Kind t3, llvm::Value * loc = nullptr) :
            kind(t),
            payload(new AType(t2, t3)),
            loc(loc) {
        }

        AType(AType const & other) = default;

        AType * merge(AType * other) {
            if (other == nullptr)
                return top;
            if (kind == Kind::B)
                return new AType(*other);
            else if (other->kind == Kind::B)
                return new AType(*this);
            switch (kind) {
                case Kind::D:
                    if (*other != Kind::D)
                        return top;
                    if (loc == other->loc)
                        return new AType(*this);
                    else
                        return new AType(Kind::D);
                case Kind::CV:
                    if (*other != Kind::CV)
                        return top;
                    if (loc == other->loc)
                        return new AType(*this);
                    else
                        return new AType(Kind::CV);
                case Kind::F:
                    if (*other != Kind::F)
                        return top;
                    if (loc == other->loc)
                        return new AType(*this);
                    else
                        return new AType(Kind::F);
                case Kind::DV:
                    if (*other != Kind::DV)
                        return top;
                    if (loc == other->loc)
                        return new AType(*this);
                    else
                        return new AType(Kind::DV, payload == nullptr ? nullptr : payload->merge(other->payload));
                case Kind::R:
                    if (*other != Kind::R)
                        return top;
                    if (loc == other->loc)
                        return new AType(*this);
                    else
                        return new AType(Kind::R, payload == nullptr ? nullptr : payload->merge(other->payload));
                default:
                    return top;
            }
        }

        AType * setValue(llvm::Value * loc) {
            this->loc = loc;
            return this;
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

        bool operator == (AType::Kind other) const {
            return kind == other;
        }

        bool operator != (AType::Kind other) const {
            return kind != other;
        }


        /** We are only interested in the R types as phi nodes are typed.
            Having a loc is smaller than not having a loc

            R(DV(D)) < R(DV) < R < T
            R(CV) < R < T
            R(F) < R < T
         */
        bool operator < (AType const & other) const {
            if (payload == nullptr and other.payload == nullptr)
                return loc != nullptr and other.loc == nullptr;
            if (payload == nullptr)
                return false;
            if (payload != nullptr and other.payload == nullptr)
                return true;
            else return *payload < *other.payload;
        }

        static AType * top;

    };

    std::ostream & operator << (std::ostream & s, AType const & t);


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

        /** For given llvm Value, returns its type. 
        
        If the value was not yet seen, assumes the bottom type. 

        */
        AType * valueType(llvm::Value * loc) {
            auto i = types_.find(loc);
            if (i == types_.end())
                return new AType(AType::Kind::B);
            else
                return i->second;
        }

        void setValueType(llvm::Value * loc, AType * type) {
            auto i = types_.find(loc);
            if (i == types_.end()) {
                changed = true;
                types_[loc] = type;
            } else {
                AType * t = i->second;
                if ((t != nullptr) and (*t < *type)) {
                    changed = true;
                    i->second = type;
                }
            }
        }

    private:

        friend std::ostream & operator << (std::ostream & s, TypeAnalysis const & ta);

        AType * genericArithmetic(llvm::CallInst * ci);
        AType * genericRelational(llvm::CallInst * ci);
        AType * genericGetElement(llvm::CallInst * ci);

        /** For each value specifies its actual type. */
        std::map<llvm::Value *, AType *> types_;

        bool changed;

    };

} // namespace rift

#endif // TYPE_ANALYSIS_H

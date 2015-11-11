#pragma once
#ifndef TYPE_ANALYSIS_H
#define TYPE_ANALYSIS_H

#include "llvm.h"

namespace rift {

    class AType {
    public:

        enum class Type {
            D,
            DV,
            CV, 
            F,
            R,
            T
        } type;

        AType * targetType;

        llvm::Value * value;

        AType(Type t, AType * targetType, llvm::Value * value = nullptr) :
            type(t),
            targetType(targetType),
            value(value) {
        }

        AType(Type t, llvm::Value * value = nullptr) :
            type(t),
            targetType(nullptr),
            value(value) {
        }

        AType(Type t, Type t2, llvm::Value * value = nullptr) :
            type(t),
            targetType(new AType(t2)),
            value(value) {
        }

        AType(Type t, Type t2, Type t3, llvm::Value * value = nullptr) :
            type(t),
            targetType(new AType(t2, t3)),
            value(value) {
        }

        AType(AType const & other) = default;

        AType * merge(AType * other) {
            if (other == nullptr)
                return top;
// seems to me we do not need this
//          if (*this == *other)
//              return new AType(*this);
            switch (type) {
                case Type::D:
                    if (*other != Type::D)
                        return top;
                    if (value == other->value)
                        return new AType(*this);
                    else
                        return new AType(Type::D);
                case Type::CV:
                    if (*other != Type::CV)
                        return top;
                    if (value == other->value)
                        return new AType(*this);
                    else
                        return new AType(Type::CV);
                case Type::F:
                    if (*other != Type::F)
                        return top;
                    if (value == other->value)
                        return new AType(*this);
                    else
                        return new AType(Type::F);
                case Type::DV:
                    if (*other != Type::DV)
                        return top;
                    if (value == other->value)
                        return new AType(*this);
                    else
                        return new AType(Type::DV, targetType->merge(other->targetType));
                case Type::R:
                    if (*other != Type::R)
                        return top;
                    if (value == other->value)
                        return new AType(*this);
                    else
                        return new AType(Type::R, targetType->merge(other->targetType));
                default:
                    return top;
            }
        }

        AType * setValue(llvm::Value * value) {
            this->value = value;
            return this;
        }

        bool isScalar() {
            if (type == Type::D)
                return true;
            else 
                return targetType != nullptr and targetType->isScalar();
        }

        bool isDouble() {
            if (type == Type::D)
                return true;
            else if (type == Type::DV)
                return true;
            else 
                return targetType != nullptr and (targetType->isDouble());
        }

        bool operator == (AType::Type other) const {
            return type == other;
        }

        bool operator != (AType::Type other) const {
            return type != other;
        }

        static AType * top;


    };

    std::ostream & operator << (std::ostream & s, AType const & t);


    /** Type and shape analysis. 
     
     Based on the calls to the runtime functions, the analysis tracks the following key attributes:

     - for each value it remembers its shape. 
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
        
        If the value was not yet seen, assumes the top type. 

        */
        AType * valueType(llvm::Value * value) {
            auto i = types_.find(value);
            if (i == types_.end())
                // envGet of variable we haven't seen yet
                return new AType(AType::Type::R);
            else
                return i->second;
        }



    private:

        friend std::ostream & operator << (std::ostream & s, TypeAnalysis const & ta);

        AType * genericArithmetic(llvm::CallInst * ci);
        AType * genericRelational(llvm::CallInst * ci);
        AType * genericGetElement(llvm::CallInst * ci);

        
        
        /** For each value specifies its actual type. */
        std::map<llvm::Value *, AType *> types_;

    };


} // namespace rift

#endif // TYPE_ANALYSIS_H
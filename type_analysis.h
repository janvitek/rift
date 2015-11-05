#pragma once
#ifndef TYPE_ANALYSIS_H
#define TYPE_ANALYSIS_H

#include "llvm.h"

namespace rift {

    class Type {
    public:
        static const Type bottom;
        /** Scalar double (primitive double type) */
        static const Type DoubleScalar;
        /** Double vector, but of length 1 - can be unboxed to scalar */
        static const Type DoubleScalarVector;
        /** Double vector of arbitrary length */
        static const Type DoubleVector;
        /** Character vector of arbitrary length */
        static const Type CharacterVector;
        /** Function object. */
        static const Type Function;
        /** Double vector of size 1 boxed into Value. */
        static const Type BoxedDoubleScalarVector;
        /** Double vector of arbitrary length boxed into Value */
        static const Type BoxedDoubleVector;
        /** Character vector of arbitrary length boxed into Value */
        static const Type BoxedCharacterVector;
        /** Function boxed into Value. */
        static const Type BoxedFunction;
        /** Any value object, Top value */
        static const Type Value;

        Type() :
            value_(Internal::Value) {}

        Type(Type const & from) = default;

        bool operator == (const Type & other) const {
            return value_ == other.value_;
        }

        bool operator != (const Type & other) const {
            return value_ != other.value_;
        }

        bool isScalar() const {
            switch (value_) {
                case Internal::DoubleScalar:
                case Internal::DoubleScalarVector:
                case Internal::BoxedDoubleScalarVector:
                    return true;
                default:
                    return false;
            }
        }

        bool isDouble() const {
            switch (value_) {
                case Internal::DoubleScalar:
                case Internal::DoubleScalarVector:
                case Internal::DoubleVector:
                case Internal::BoxedDoubleScalarVector:
                case Internal::BoxedDoubleVector:
                    return true;
                default:
                    return false;
            }
        }

        bool isCharacter() const {
            switch (value_) {
                case Internal::CharacterVector:
                case Internal::BoxedCharacterVector:
                    return true;
                default:
                    return false;
            }
        }

        bool isFunction() const {
            switch (value_) {
                case Internal::Function:
                case Internal::BoxedFunction:
                    return true;
                default:
                    return false;
            }
        }

        bool isBoxedDouble() const {
            switch (value_) {
                case Internal::BoxedDoubleScalarVector:
                case Internal::BoxedDoubleVector:
                    return true;
                default:
                    return false;
            }
        }

        bool isDifferentClassAs(Type const & other) {
            if (isDouble() and not other.isDouble())
                return true;
            if (isCharacter() and not other.isCharacter())
                return true;
            if (isFunction() and not other.isFunction())
                return true;
            return false;
        }

    private:
        enum class Internal {
            DoubleScalar,
            DoubleScalarVector,
            DoubleVector,
            CharacterVector,
            Function,
            BoxedDoubleScalarVector,
            BoxedDoubleVector,
            BoxedCharacterVector,
            BoxedFunction,
            Value
        };
        Type(Internal value) :
            value_(value) {}
        Internal value_;



    };


    class TypeAnalysis : public llvm::FunctionPass {
    public:

        static char ID;

        char const * getPassName() const override {
            return "TypeAnalysis";
        }

        TypeAnalysis() : llvm::FunctionPass(ID) {}

        bool runOnFunction(llvm::Function & f) override;

        Type valueType(llvm::Value * value) {
            auto i = types_.find(value);
            if (i == types_.end())
                return Type::Value;
            else
                return i->second;
        }

        void setValueType(llvm::Value * value, Type t) {
            types_[value] = t;
        }

        void setAsBoxed(llvm::Value * boxed, llvm::Value * unboxed) {
            unboxed_[boxed] = unboxed;
        }

        llvm::Value * unbox(llvm::Value * value) {
            auto i = unboxed_.find(value);
            if (i == unboxed_.end())
                return nullptr;
            return i->second;
        }

        llvm::Value * unboxAll(llvm::Value * value) {
            while (true) {
                auto i = unboxed_.find(value);
                if (i == unboxed_.end())
                    return value;
                else
                    value = i->second;
            }
        }

    private:
        Type genericAdd(llvm::CallInst * ci);

        Type genericGetElement(llvm::CallInst * ci);

        Type doubleOperator(llvm::CallInst * ci);

        Type comparisonOperator(llvm::CallInst * ci);


        /** For each value specifies its actual type. */
        std::map<llvm::Value *, Type> types_;
        /** For each value, lists the unboxed version of it, if present. */
        std::map<llvm::Value *, llvm::Value *> unboxed_;

    };


} // namespace rift

#endif // TYPE_ANALYSIS_H
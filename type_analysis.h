#pragma once
#ifndef TYPE_ANALYSIS_H
#define TYPE_ANALYSIS_H

#include "llvm.h"

namespace rift {

    /** Determines the type of a Value in rift. 
    
    Reflects both information about the type (Value, DoubleVector, CharacterVector, Function, DoubleScalar) and the shape in case of Values and doubles where the type also tells us the underlying type of the value, if one is known, as well as the shape of double vectors - i.e. if they are vectors or scalars. 
    
     */
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

        /** Returns true if the shape of the value is double scalar. 
         */
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

        /** Returns true if the shape of the value is double vector (including scalars). 
         */
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

        /** Returns true if the shape of the value is character vector. 
         */
        bool isCharacter() const {
            switch (value_) {
                case Internal::CharacterVector:
                case Internal::BoxedCharacterVector:
                    return true;
                default:
                    return false;
            }
        }

        /** Returns true if the shape of the value is function pointer. 
         */
        bool isFunction() const {
            switch (value_) {
                case Internal::Function:
                case Internal::BoxedFunction:
                    return true;
                default:
                    return false;
            }
        }

        /** Returns true if the Value is boxed double vector (or scalar) 
         */
        bool isBoxedDouble() const {
            switch (value_) {
                case Internal::BoxedDoubleScalarVector:
                case Internal::BoxedDoubleVector:
                    return true;
                default:
                    return false;
            }
        }

        /** Returns true if the two value types and definitely distinctive (distinctive types are Doubles, Characters and Functions). 
         */
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
        Type valueType(llvm::Value * value) {
            auto i = types_.find(value);
            if (i == types_.end())
                return Type::Value;
            else
                return i->second;
        }

        /** Sets type for given llvm value. 
         */
        void setValueType(llvm::Value * value, Type t) {
            types_[value] = t;
        }

        /** Stores the pairing between boxed and unboxed llvm values. 
         */
        void setAsBoxed(llvm::Value * boxed, llvm::Value * unboxed) {
            unboxed_[boxed] = unboxed;
        }
        
        /** Unboxes the given value by one step (i.e. from Value DoubleVector, DoubleScalarVector, CharacterVector, or Function, or DoubleScalar from DoubleScalarVector
         */
        llvm::Value * unbox(llvm::Value * value) {
            auto i = unboxed_.find(value);
            if (i == unboxed_.end())
                return nullptr;
            return i->second;
        }


    private:
        /** Type analysis of the add call. 
         */
        Type genericAdd(llvm::CallInst * ci);

        /** Type analysis of getElement call. 
        */
        Type genericGetElement(llvm::CallInst * ci);

        /** Type analysis of double operators. 
         */
        Type doubleOperator(llvm::CallInst * ci);

        /** Type analysis on comparison operators. 
         */
        Type comparisonOperator(llvm::CallInst * ci);

        /** For each value specifies its actual type. */
        std::map<llvm::Value *, Type> types_;
        /** For each value, lists the unboxed version of it, if present. */
        std::map<llvm::Value *, llvm::Value *> unboxed_;

    };


} // namespace rift

#endif // TYPE_ANALYSIS_H
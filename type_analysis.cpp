#include <iostream>
#include <ciso646>

#include "type_analysis.h"
#include "compiler.h"

using namespace std;
using namespace llvm;

namespace rift {

    const Type Type::DoubleScalar(Type::Internal::DoubleScalar);
    const Type Type::DoubleScalarVector(Type::Internal::DoubleScalarVector);
    const Type Type::DoubleVector(Type::Internal::DoubleVector);
    const Type Type::CharacterVector(Type::Internal::CharacterVector);
    const Type Type::Function(Type::Internal::Function);
    const Type Type::BoxedDoubleScalarVector(Type::Internal::BoxedDoubleScalarVector);
    const Type Type::BoxedDoubleVector(Type::Internal::BoxedDoubleVector);
    const Type Type::BoxedCharacterVector(Type::Internal::BoxedCharacterVector);
    const Type Type::BoxedFunction(Type::Internal::BoxedFunction);
    const Type Type::Value(Type::Internal::Value);

    char TypeAnalysis::ID = 0;

    Type TypeAnalysis::genericAdd(CallInst * ci) {
        Type lhs = valueType(ci->getOperand(0));
        Type rhs = valueType(ci->getOperand(1));
        if (lhs.isScalar() and rhs.isScalar())
            return Type::BoxedDoubleScalarVector;
        else if (lhs.isDouble() and rhs.isDouble())
            return Type::BoxedDoubleVector;
        else if (lhs.isCharacter() and rhs.isCharacter())
            return Type::BoxedCharacterVector;
        else if (lhs.isFunction() or rhs.isFunction())
            throw "Invalid types for binary add operator";
        else
            return Type::Value;
    }

    Type TypeAnalysis::genericGetElement(CallInst * ci) {
        Type from = valueType(ci->getOperand(0));
        Type index = valueType(ci->getOperand(1));
        if (index.isFunction() or index.isCharacter())
            throw "Index must be double";
        if (from.isCharacter()) {
            return Type::BoxedCharacterVector;
        } else if (from.isDouble()) {
            if (index.isScalar())
                return Type::BoxedDoubleScalarVector;
            else
                return Type::BoxedDoubleVector;
        } else if (from == Type::Value) {
            return Type::Value;
        } else {
            throw "Cannot index a function";
        }
    }

    Type TypeAnalysis::doubleOperator(CallInst * ci) {
        Type lhs = valueType(ci->getOperand(0));
        Type rhs = valueType(ci->getOperand(1));
        if (lhs.isScalar() and rhs.isScalar())
            return Type::BoxedDoubleScalarVector;
        else if (lhs.isDouble() and rhs.isDouble())
            return Type::BoxedDoubleVector;
        else if (lhs.isFunction() or rhs.isFunction() or lhs.isCharacter() or rhs.isCharacter())
            throw "Invalid types for binary operator";
        else
            return Type::Value;
    }

    Type TypeAnalysis::comparisonOperator(CallInst * ci) {
        Type lhs = valueType(ci->getOperand(0));
        Type rhs = valueType(ci->getOperand(1));
        if (lhs.isScalar() and rhs.isScalar())
            return Type::BoxedDoubleScalarVector;
        else if (lhs.isDouble() and rhs.isDouble())
            return Type::BoxedDoubleVector;
        else if (lhs.isCharacter() and rhs.isCharacter())
            return Type::BoxedDoubleVector;
        else if (lhs == Type::Value and rhs == Type::Value)
            return Type::BoxedDoubleVector;
        else
            return Type::BoxedDoubleScalarVector;
    }

    bool TypeAnalysis::runOnFunction(llvm::Function & f) {
        types_.clear();
        //std::cout << "runnning TypeAnalysis..." << std::endl;
        // for all basic blocks, for all instructions
        for (auto & b : f) {
            for (auto & i : b) {
                if (CallInst * ci = dyn_cast<CallInst>(&i)) {
                    StringRef s = ci->getCalledFunction()->getName();
                    if (s == "doubleVectorLiteral") {
                        // when creating literal from a double, it is always double scalar
                        types_[ci] = Type::DoubleScalarVector;
                        types_[ci->getOperand(0)] = Type::DoubleScalar;
                        unboxed_[ci] = ci->getOperand(0);
                    } else if (s == "characterVectorLiteral") {
                        types_[ci] = Type::CharacterVector;
                        unboxed_[ci] = ci->getOperand(0);
                    } else if (s == "fromDoubleVector") {
                        // based on whether the value is created from boxed scalar or boxed vector
                        types_[ci] = valueType(ci->getOperand(0)) == Type::DoubleScalarVector ? Type::BoxedDoubleScalarVector : Type::BoxedDoubleVector;
                        unboxed_[ci] = ci->getOperand(0);
                    } else if (s == "fromCharacterVector") {
                        types_[ci] = Type::BoxedCharacterVector;
                        unboxed_[ci] = ci->getOperand(0);
                    } else if (s == "fromFunction") {
                        types_[ci] = Type::BoxedFunction;
                        unboxed_[ci] = ci->getOperand(0);
                    } else if (s == "genericGetElement") {
                        types_[ci] = genericGetElement(ci);
                    } else if (s == "genericSetElement") {
                        // don't do anything for set element as it does not produce any new value
                    } else if (s == "genericAdd") {
                        types_[ci] = genericAdd(ci);
                    } else if (s == "genericSub") {
                        types_[ci] = doubleOperator(ci);
                    } else if (s == "genericMul") {
                        types_[ci] = doubleOperator(ci);
                    } else if (s == "genericDiv") {
                        types_[ci] = doubleOperator(ci);
                    } else if (s == "genericEq") {
                        types_[ci] = comparisonOperator(ci);
                    } else if (s == "genericNeq") {
                        types_[ci] = comparisonOperator(ci);
                    } else if (s == "genericLt") {
                        types_[ci] = doubleOperator(ci);
                    } else if (s == "genericGt") {
                        types_[ci] = doubleOperator(ci);
                    } else if (s == "length") {
                        // result of length operation is always double scalar
                        types_[ci] = Type::DoubleScalar;
                    } else if (s == "type") {
                        // result of type operation is always character vector
                        types_[ci] = Type::CharacterVector;
                    } else if (s == "c") {
                        // make sure the types to c are correct
                        Type t = valueType(ci->getArgOperand(1));
                        if (t.isFunction())
                            throw "Cannot concatenate functions";
                        for (unsigned i = 2; i < ci->getNumArgOperands(); ++i) {
                            Type tt = valueType(ci->getArgOperand(i));
                            if (tt.isFunction())
                                throw "Cannot concatenate functions";
                            if (t == Type::Value) {
                                t = tt;
                            } else if (t.isDifferentClassAs(tt))
                                throw "Cannot concatenate different types";
                        }
                        if (t.isDouble())
                            types_[ci] = Type::BoxedDoubleVector;
                        else if (t.isCharacter())
                            types_[ci] = Type::BoxedCharacterVector;
                        else
                            types_[ci] = Type::Value;
                    } else if (s == "genericEval") {
                        Type t = valueType(ci->getOperand(0));
                        if (t != Type::Value and not t.isCharacter())
                            throw "Invalid type for eval";
                        types_[ci] = Type::Value;
                    } else if (s == "envGet") {
                        types_[ci] = types_[ci->getOperand(1)];
                    } else if (s == "envSet") {
                        types_[ci->getOperand(1)] = types_[ci->getOperand(2)];
                    }
                }
            }
        }
        return false;
    }

} // namespace rift
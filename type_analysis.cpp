#include <iostream>
#include <ciso646>

#include "type_analysis.h"
#include "compiler.h"

using namespace std;
using namespace llvm;

namespace rift {

    AType * AType::top = nullptr; // new AType(AType::Type::T);


    char TypeAnalysis::ID = 0;

    AType * TypeAnalysis::genericArithmetic(CallInst * ci) {
        AType * lhs = valueType(ci->getOperand(0));
        AType * rhs = valueType(ci->getOperand(1));
        return lhs->merge(rhs)->setValue(ci);
    }

    AType * TypeAnalysis::genericRelational(CallInst * ci) {
        AType * lhs = valueType(ci->getOperand(0));
        AType * rhs = valueType(ci->getOperand(1));
        if (lhs->isScalar() and rhs->isScalar())
            return new AType(AType::Type::R, AType::Type::DV, AType::Type::D, ci);
        else
            return new AType(AType::Type::R, AType::Type::DV, ci);
    }


    AType * TypeAnalysis::genericGetElement(CallInst * ci) {
        AType * from = valueType(ci->getOperand(0));
        AType * index = valueType(ci->getOperand(1));
        if (from->isDouble()) {
            if (index->isScalar()) {
                AType * s = new AType(AType::Type::D);
                AType * v = new AType(AType::Type::DV, s);
                return new AType(AType::Type::R, v, ci);
                return new AType(AType::Type::R, AType::Type::DV, AType::Type::D, ci);
            } else {
                AType * v = new AType(AType::Type::DV);
                return new AType(AType::Type::R, v, ci);
                return new AType(AType::Type::R, AType::Type::DV, ci);
            }
        } else {
            AType * v = new AType(AType::Type::CV);
            return new AType(AType::Type::R, v, ci);
        }
    }

    bool TypeAnalysis::runOnFunction(llvm::Function & f) {
        types_.clear();
        //std::cout << "runnning TypeAnalysis..." << std::endl;
        // for all basic blocks, for all instructions
        do {
            // cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << endl;
            changed = false;
            for (auto & b : f) {
                for (auto & i : b) {
                    if (CallInst * ci = dyn_cast<CallInst>(&i)) {
                        StringRef s = ci->getCalledFunction()->getName();
                        if (s == "doubleVectorLiteral") {
                            // when creating literal from a double, it is always double scalar
                            llvm::Value * op = ci->getOperand(0);
                            setValueType(op, new AType(AType::Type::D, op));
                            setValueType(ci, new AType(AType::Type::DV, valueType(op), ci));
                        } else if (s == "characterVectorLiteral") {
                            setValueType(ci, new AType(AType::Type::CV, ci));
                        } else if (s == "fromDoubleVector") {
                            setValueType(ci, new AType(AType::Type::R, valueType(ci->getOperand(0)), ci));
                        } else if (s == "fromCharacterVector") {
                            setValueType(ci, new AType(AType::Type::R, valueType(ci->getOperand(0)), ci));
                        } else if (s == "fromFunction") {
                            setValueType(ci, new AType(AType::Type::R, valueType(ci->getOperand(0)), ci));
                        } else if (s == "genericGetElement") {
                            setValueType(ci, genericGetElement(ci));
                        } else if (s == "genericSetElement") {
                            // don't do anything for set element as it does not produce any new value
                        } else if (s == "genericAdd") {
                            setValueType(ci, genericArithmetic(ci));
                        } else if (s == "genericSub") {
                            setValueType(ci, genericArithmetic(ci));
                        } else if (s == "genericMul") {
                            setValueType(ci, genericArithmetic(ci));
                        } else if (s == "genericDiv") {
                            setValueType(ci, genericArithmetic(ci));
                        } else if (s == "genericEq") {
                            setValueType(ci, genericRelational(ci));
                        } else if (s == "genericNeq") {
                            setValueType(ci, genericRelational(ci));
                        } else if (s == "genericLt") {
                            setValueType(ci, genericRelational(ci));
                        } else if (s == "genericGt") {
                            setValueType(ci, genericRelational(ci));
                        } else if (s == "length") {
                            // result of length operation is always double scalar
                            setValueType(ci, new AType(AType::Type::D, ci));
                        } else if (s == "type") {
                            // result of type operation is always character vector
                            setValueType(ci, new AType(AType::Type::CV, ci));
                        } else if (s == "c") {
                            // make sure the types to c are correct
                            AType * t1 = valueType(ci->getArgOperand(1));
                            for (unsigned i = 2; i < ci->getNumArgOperands(); ++i)
                                t1 = t1->merge(valueType(ci->getArgOperand(i)));
                            if (t1->isScalar())
                                // concatenation of scalars is a vector
                                t1 = new AType(AType::Type::R, AType::Type::DV, ci);
                            else
                                // c(c(1,2), c(2,3) !
                                t1->setValue(ci);
                            setValueType(ci, t1);
                        } else if (s == "genericEval") {
                            setValueType(ci, new AType(AType::Type::R, ci));
                        } else if (s == "envGet") {
                            setValueType(ci, new AType(AType::Type::R, ci));
                        } else if (s == "envSet") {
                            setValueType(ci->getOperand(1), valueType(ci->getOperand(2)));
                        }
                    } else if (PHINode * phi = dyn_cast<PHINode>(&i)) {
                        AType * first = valueType(phi->getOperand(0));
                        AType * second = valueType(phi->getOperand(1));
                        AType * result = first->merge(second)->setValue(phi);

                        setValueType(phi, result);
                    }
                }
            }
        } while (changed);
        //f.dump();
        //cout << *this << endl;
        return false;
    }

    std::ostream & operator << (std::ostream & s, AType const & t) {
        llvm::raw_os_ostream ss(s);
        if (t.value != nullptr)
            t.value->printAsOperand(ss, false);
        switch (t.type) {
            case AType::Type::D:
                ss << " D ";
                break;
            case AType::Type::DV:
                ss << " DV ";
                break;
            case AType::Type::CV:
                ss << " CV ";
                break;
            case AType::Type::F:
                ss << " F ";
                break;
            case AType::Type::R:
                ss << " R ";
                break;
            case AType::Type::T:
                ss << " T ";
                break;
        }
        ss.flush();
        if (t.targetType != nullptr)
            s << " -> " << *(t.targetType);
        return s;
    }


    std::ostream & operator << (std::ostream & s, TypeAnalysis const & ta) {
        s << "Type Analysis: " << "\n";
        for (auto const & v : ta.types_) {
            llvm::Value * vv = v.first;
            s << *v.second << endl;
        }
        return s;
    }


} // namespace rift


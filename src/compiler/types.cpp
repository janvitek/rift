#include "types.h"
#include "compiler.h"

namespace rift {

namespace type {

/** Initialization type declarations. Each Rift type must be declared to
    LLVM.
  */
#define STRUCT(name, ...) \
    llvm::StructType::create(name, __VA_ARGS__, nullptr)
#define FUN_TYPE(result, ...) \
    llvm::FunctionType::get(result, std::vector<llvm::Type*>({ __VA_ARGS__}), false)
#define FUN_TYPE_VARARG(result, ...) \
    llvm::FunctionType::get(result, std::vector<llvm::Type*>({ __VA_ARGS__}), true)

llvm::StructType * environmentType();

llvm::Type * Void = llvm::Type::getVoidTy(Compiler::context());
llvm::Type * Int = llvm::IntegerType::get(Compiler::context(), 32);
llvm::Type * Double = llvm::Type::getDoubleTy(Compiler::context());
llvm::Type * Character = llvm::IntegerType::get(Compiler::context(), 8);
llvm::Type * Bool = llvm::IntegerType::get(Compiler::context(), 1);

llvm::PointerType * ptrInt = llvm::PointerType::get(Int, 0);
llvm::PointerType * ptrCharacter = llvm::PointerType::get(Character, 0);
llvm::PointerType * ptrDouble = llvm::PointerType::get(Double, 0);

llvm::StructType * DoubleVector = STRUCT("DoubleVector", ptrDouble, Int);
llvm::StructType * CharacterVector = STRUCT("CharacterVector", ptrCharacter, Int);
llvm::PointerType * ptrDoubleVector = llvm::PointerType::get(DoubleVector, 0);
llvm::PointerType * ptrCharacterVector = llvm::PointerType::get(CharacterVector, 0);

llvm::StructType * Value = STRUCT("Value", Int, ptrDoubleVector);
llvm::PointerType * ptrValue = llvm::PointerType::get(Value, 0);

llvm::StructType * Binding = STRUCT("Binding", Int, ptrValue);
llvm::PointerType * ptrBinding = llvm::PointerType::get(Binding, 0);

llvm::PointerType * ptrEnvironment;
llvm::StructType * Environment = environmentType();

llvm::FunctionType * NativeCode = FUN_TYPE(ptrValue, ptrEnvironment);

llvm::StructType * Function = STRUCT("Function", ptrEnvironment, NativeCode, ptrInt, Int);
llvm::PointerType * ptrFunction = llvm::PointerType::get(Function, 0);

llvm::FunctionType * v_i = FUN_TYPE(ptrValue, Int);
llvm::FunctionType * v_dv = FUN_TYPE(ptrValue, ptrDoubleVector);
llvm::FunctionType * v_d = FUN_TYPE(ptrValue, Double);
llvm::FunctionType * v_cv = FUN_TYPE(ptrValue, ptrCharacterVector);
llvm::FunctionType * v_ev = FUN_TYPE(ptrValue, ptrEnvironment, ptrValue);
llvm::FunctionType * v_v = FUN_TYPE(ptrValue, ptrValue);
llvm::FunctionType * v_vv = FUN_TYPE(ptrValue, ptrValue, ptrValue);
llvm::FunctionType * v_vvv = FUN_TYPE(ptrValue, ptrValue, ptrValue, ptrValue);
llvm::FunctionType * v_vi = FUN_TYPE(ptrValue, ptrValue, Int);
llvm::FunctionType * v_viv = FUN_TYPE(ptrValue, ptrValue, Int, ptrValue);
llvm::FunctionType * v_ei = FUN_TYPE(ptrValue, ptrEnvironment, Int);
llvm::FunctionType * v_ecv = FUN_TYPE(ptrValue, ptrEnvironment, ptrCharacterVector);
llvm::FunctionType * v_cvcv = FUN_TYPE(ptrValue, ptrCharacterVector, ptrCharacterVector);
llvm::FunctionType * v_dvdv = FUN_TYPE(ptrValue, ptrDoubleVector, ptrDoubleVector);
llvm::FunctionType * void_eiv = FUN_TYPE(Void, ptrEnvironment, Int, ptrValue);
llvm::FunctionType * d_dvd = FUN_TYPE(Double, ptrDoubleVector, Double);

llvm::FunctionType * v_f = FUN_TYPE(ptrValue, ptrFunction);
llvm::FunctionType * v_ie = FUN_TYPE(ptrValue, Int, ptrEnvironment);

llvm::FunctionType * b_v = FUN_TYPE(Bool, ptrValue);

llvm::FunctionType * v_viVA = FUN_TYPE_VARARG(ptrValue, ptrValue, Int);

llvm::FunctionType * v_cvdv = FUN_TYPE(ptrValue, ptrCharacterVector, ptrDoubleVector);

llvm::FunctionType * void_vvv = FUN_TYPE(Void, ptrValue, ptrValue, ptrValue);
llvm::FunctionType * void_dvdvdv = FUN_TYPE(Void, ptrDoubleVector, ptrDoubleVector, ptrDoubleVector);
llvm::FunctionType * void_cvdvcv = FUN_TYPE(Void, ptrCharacterVector, ptrDoubleVector, ptrCharacterVector);
llvm::FunctionType * void_dvdd = FUN_TYPE(Void, ptrDoubleVector, Double, Double);

llvm::FunctionType * d_v = FUN_TYPE(Double, ptrValue);
llvm::FunctionType * v_iVA = FUN_TYPE_VARARG(ptrValue, Int);

llvm::FunctionType * d_dv = FUN_TYPE(Double, ptrDoubleVector);
llvm::FunctionType * f_v = FUN_TYPE(ptrFunction, ptrValue);

llvm::StructType * environmentType() {
    llvm::StructType * result = llvm::StructType::create(Compiler::context(), "Environment");
    ptrEnvironment = llvm::PointerType::get(result, 0);
    result->setBody(ptrEnvironment, ptrBinding, Int, nullptr);
    return result;
}

}

}

#pragma once

#include "llvm.h"

namespace rift {

namespace type {

extern llvm::Type * Void;
extern llvm::Type * Int;
extern llvm::Type * Double;
extern llvm::Type * Character;
extern llvm::Type * Bool;

extern llvm::PointerType * ptrInt;
extern llvm::PointerType * ptrCharacter;
extern llvm::PointerType * ptrDouble;

extern llvm::StructType *  DoubleVector;
extern llvm::StructType *  CharacterVector;
extern llvm::PointerType * ptrDoubleVector;
extern llvm::PointerType * ptrCharacterVector;

/** Unions in llvm are represented by the longest members, all others are
    obtained by casting.
*/
extern llvm::StructType *  Value;
extern llvm::PointerType * ptrValue;

extern llvm::StructType *  Binding;
extern llvm::PointerType * ptrBinding;

extern llvm::PointerType * ptrEnvironment;
extern llvm::StructType *  Environment;

extern llvm::FunctionType * NativeCode;

extern llvm::StructType *  Function;
extern llvm::PointerType * ptrFunction;


/** Declaration of runtime function types. The naming convention is "x_yz"
    where x is the return type of the function and y and z are arguments. VA
    is used for varargs so that the type of the function can be easily
    determined from the name:
      d = double scalar
      dv = double vector *
      i = integer
      cv = character vector *
      e = Environment *
      v = Value *
      f = Function *
  */
extern llvm::FunctionType * v_i;
extern llvm::FunctionType * v_dv;
extern llvm::FunctionType * v_d;
extern llvm::FunctionType * v_cv;
extern llvm::FunctionType * v_ev;
extern llvm::FunctionType * v_v;
extern llvm::FunctionType * v_vv;
extern llvm::FunctionType * v_vvv;
extern llvm::FunctionType * v_vi;
extern llvm::FunctionType * v_viv;
extern llvm::FunctionType * v_ei;
extern llvm::FunctionType * v_ecv;
extern llvm::FunctionType * v_cvcv;
extern llvm::FunctionType * v_dvdv;
extern llvm::FunctionType * void_eiv;
extern llvm::FunctionType * d_dvd;
extern llvm::FunctionType * v_f;
extern llvm::FunctionType * v_ie;
extern llvm::FunctionType * b_v;
extern llvm::FunctionType * v_viVA;
extern llvm::FunctionType * void_vvv;
extern llvm::FunctionType * void_dvdvdv;
extern llvm::FunctionType * void_dvdd;
extern llvm::FunctionType * void_cvdvcv;
extern llvm::FunctionType * v_cvdv;
extern llvm::FunctionType * d_v;
extern llvm::FunctionType * v_iVA;
extern llvm::FunctionType * d_dv;
extern llvm::FunctionType * f_v;
}
}

#include "parse.h"

using namespace llvm;

namespace {

Type * initializeTypes();

Type * t_I = initializeTypes();
Type * t_D;
PointerType * p_DV;
FunctionType * DV__D;
FunctionType * DV__varI;
FunctionType * V__DV;
FunctionType * D__DV;
FunctionType * D__DV_D;
FunctionType * V__DV_D_D;

Function * f_createDV;
Function * f_concatDV;
Function * f_deleteDV;
Function * f_setDVElem;
Function * f_getDVElem;
Function * f_getDVSize;


Type * initializeTypes() {
    // create the type declarations required in the runtime functions
    t_I = IntegerType::get(getGlobalContext(), 32);
    t_D = Type::getDoubleTy(getGlobalContext());
    StructType * t_DV = StructType::create(getGlobalContext(), "DV");
    std::vector<Type *> fields;
    fields.push_back(t_I); 
    fields.push_back(PointerType::get(t_D, 0));
    t_DV->setBody(fields, false);
    p_DV = PointerType::get(t_DV, 0);
    // create the function types from the API
    fields = { t_D };
    DV__D = FunctionType::get(p_DV, fields, false);
    fields = { t_I };
    DV__varI = FunctionType::get(p_DV, fields, true);
    fields = { p_DV };
    V__DV = FunctionType::get(Type::getVoidTy(getGlobalContext()), 
			      fields, false);
    D__DV = FunctionType::get(t_D, fields, false);
    fields = { p_DV, t_D };
    D__DV_D = FunctionType::get(t_D, fields, false);
    fields = { p_DV, t_D, t_D };
    V__DVdoubledouble = FunctionType::get(Type::getVoidTy(getGlobalContext()), 
					  fields, false);
    return t_I;
}

using Function;

Module * initializeModule(std::string const & name) {
    Module * m = new Module(name, getGlobalContext());
    // and finally create the function declarations
    f_createDV = Create(DV__D, ExternalLinkage, "createDV", m);
    f_concatDV = Create(DV__varI, ExternalLinkage, "concatDV", m);
    f_deleteDV = Create(V__DV, ExternalLinkage, "deleteDV", m);
    f_setDVElem = Create(V__DV_D_D, ExternalLinkage, "setDVElem", m);
    f_getDVElem = Create(D__DV_D, ExternalLinkage, "getDVElem", m);
    f_getDVSize = Create(D__DV, ExternalLinkage, "getDVSize", m);
    return m;
}

#define ARGS(...) std::vector<Value *>({ __VA_ARGS__ })

class Compiler {
  Module * m;
  BasicBlock * bb;

  /** Numeric constant is converted to a vector of size 1. Better way
      would be to have a single API call for it, but this nicely shows how
      to actually work with the calls.
  */
  Value * compile(Num const & e) {
    // create a vector of size 1
    Value * result = CallInst::Create(f_createDV, ARGS(constant(1)), 
				      "", bb);
    // set its first element to the given double
    CallInst::Create(f_setDVElem, ARGS(result, constant(0), 
				       constant(e.value)), 
		     "", bb);
    return result;
  }

  /** Compiles a call to c() function.
   */
  Value * compileC(Call const & e) {
    std::vector<Value *> args;
    args.push_back(constant(static_cast<int>(e.args->size()))); 
    // number of arguments
    for (Exp * arg : *e.args)
      args.push_back(arg->codegen()); 
    // this will not be codegen but a visitor pattern
    return CallInst::Create(f_concatDV, args, "", bb);
  }

  Value * constant(int value) {
    return ConstantInt::get(getGlobalContext(), APInt(value, 32));
  }

    Value * constant(double value) {
        return ConstantFP::get(getGlobalContext(), APFloat(value));
    }
};


} // namespace

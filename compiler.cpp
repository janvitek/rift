#include "parse.h"

using namespace llvm;

namespace {

Type * initializeTypes();

Type * t_Int = initializeTypes();
Type * t_Double;
PointerType * p_DV;
FunctionType * dv_double;
FunctionType * dv_intvargs;
FunctionType * void_dv;
FunctionType * double_dv;
FunctionType * double_dvdouble;
FunctionType * void_dvdoubledouble;

Function * f_createDV;
Function * f_concatDV;
Function * f_deleteDV;
Function * f_setDVElem;
Function * f_getDVElem;
Function * f_getDVSize;

Type * initializeTypes() {
    // now we must create the type declarations required in the runtime functions
    t_Int = IntegerType::get(getGlobalContext(), 32);
    t_Double = Type::getDoubleTy(getGlobalContext());
    StructType * t_DV = StructType::create(getGlobalContext(), "DV");
    std::vector<Type *> fields;
    fields.push_back(t_Int); // length
    fields.push_back(PointerType::get(t_Double, 0));
    t_DV->setBody(fields, false);
    p_DV = PointerType::get(t_DV, 0);
    // create the function types from the API
    fields = { t_Double };
    dv_double = FunctionType::get(p_DV, fields, false);
    fields = { t_Int };
    dv_intvargs = FunctionType::get(p_DV, fields, true);
    fields = { p_DV };
    void_dv = FunctionType::get(Type::getVoidTy(getGlobalContext()), fields, false);
    double_dv = FunctionType::get(t_Double, fields, false);
    fields = { p_DV, t_Double };
    double_dvdouble = FunctionType::get(t_Double, fields, false);
    fields = { p_DV, t_Double, t_Double };
    void_dvdoubledouble = FunctionType::get(Type::getVoidTy(getGlobalContext()), fields, false);
    return t_Int;
}

Module * initializeModule(std::string const & name) {
    Module * m = new Module(name, getGlobalContext());
    // and finally create the function declarations
    f_createDV = Function::Create(dv_double, Function::ExternalLinkage, "createDV", m);
    f_concatDV = Function::Create(dv_intvargs, Function::ExternalLinkage, "concatDV", m);
    f_deleteDV = Function::Create(void_dv, Function::ExternalLinkage, "deleteDV", m);
    f_setDVElem = Function::Create(void_dvdoubledouble, Function::ExternalLinkage, "setDVElem", m);
    f_getDVElem = Function::Create(double_dvdouble, Function::ExternalLinkage, "getDVElem", m);
    f_getDVSize = Function::Create(double_dv, Function::ExternalLinkage, "getDVSize", m);
    return m;
}


#define ARGS(...) std::vector<Value *>({ __VA_ARGS__ })

class Compiler {
    Module * m;
    BasicBlock * bb;


    /** Numeric constant is converted to a vector of size 1. Better way would be to have a single API call for it, but this nicely shows how to actually work with the calls.
      */
    Value * compile(Num const & e) {
        // create a vector of size 1
        Value * result = CallInst::Create(f_createDV, ARGS(constant(1)), "", bb);
        // set its first element to the given double
        CallInst::Create(f_setDVElem, ARGS(result, constant(0), constant(e.value)), "", bb);
        return result;
    }

    /** Compiles a call to c() function.
     */
    Value * compileC(Call const & e) {
        std::vector<Value *> args;
        args.push_back(constant(static_cast<int>(e.args->size()))); // number of arguments
        for (Exp * arg : *e.args)
            args.push_back(arg->codegen()); // this will not be codegen but a visitor pattern
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

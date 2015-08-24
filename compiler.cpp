#include "llvm/IR/Verifier.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "parse.h"


namespace {

using namespace llvm;

using Function;

#define DEF_BASE_TYPE( name , code ) \
  Type *init_ ## name() { return code; }  \
  Type * name = init_ ## name();


#define DEF_FUNCTION( name, signature ) \
  Function *fun_ ## name; \
  void init_function_ ## name (Module *module) {  \
    fun_ ## name = Create( signature, ExternalLinkage,\
                             " ## name ## ", module); \
  }

#define DEF_FUNTYPE1( returntype , argtype )	\
  FunctionType*  init_funtype_ ## returntype ## __ ## argtype () { \
     std::vector<Type *> args; \
     args = { argtype }; \
     return FunctionType::get( returntype, args, false ); \
  } \
  FunctionType * funtype_ ## returntype ## __ ## argtype = 	\
    init_funtype_ ## returntype ## __ ## argtype();

#define DEF_FUNTYPE2( returntype , at1, at2 )			\
  FunctionType* init_funtype_ ## returntype ## __ ## at1 ## _ ## at2 () { \
     std::vector<Type *> args; \
     args = { at1, at2 };			       \
     return FunctionType::get( returntype, args, false ); \
  } \
  FunctionType * funtype_ ## returntype ## __ ## at1 ## _ ## at2 = \
    init_funtype_ ## returntype ## __ ## at1 ## _ ## at2();

#define DEF_FUNTYPE3( returntype , at1, at2, at3 )			 \
  FunctionType* \
   init_funtype_ ## returntype ## __ ## at1 ## _ ## at2 ## _ ## at3() { \
     std::vector<Type *> args; \
     args = { at1, at2, at3 };				  \
     return FunctionType::get( returntype, args, false ); \
  } \
  FunctionType * funtype_ ## returntype ## __ ## at1 ## _ ## at2 ## _ ## at3 = \
    init_funtype_ ## returntype ## __ ## at1 ## _ ## at2 ## _ ## at3();
 
#define DEF_STRUCT2( name , a1, a2 )	\
  StructType * init_ ## name () { \
    StructType * t = StructType::create(getGlobalContext(), " ## E ## ");\
    std::vector<Type *> fields; \
    fields.push_back(a1);\
    fields.push_back(a2);\
    t->setBody(fields, false);\
    return t;\
  } \
  StructType * name = init_ ## name();

#define DEF_STRUCT4( name , a1, a2, a3, a4 )	\
  StructType * init_ ## name () { \
    StructType * t = StructType::create(getGlobalContext(), " ## E ## ");\
    std::vector<Type *> fields; \
    fields.push_back(a1);\
    fields.push_back(a2);\
    fields.push_back(a3);\
    fields.push_back(a4);\
    t->setBody(fields, false);\
    return t;\
  } \
  StructType * name = init_ ## name();

PointerType *pE;
PointerType *pRV;
PointerType *pCV;
PointerType *pDV;
PointerType *pF;
PointerType *pB;  // Bindings
PointerType *pC;
PointerType *pD;

PointerType *varC;
PointerType *varD;

DEF_BASE_TYPE( V,  Type::getVoidTy(getGlobalContext()));
DEF_BASE_TYPE( I,  IntegerType::get(getGlobalContext(), 32));
DEF_BASE_TYPE( D,  Type::getDoubleTy(getGlobalContext()));
DEF_BASE_TYPE( C,  Type::getDoubleTy(getGlobalContext()));

DEF_BASE_TYPE( pV, PointerType::get(V, 0));

DEF_STRUCT4( E, pE, pB, I, I);
DEF_STRUCT2( B, pV, pRV);
DEF_STRUCT2( F, pE, pV);
DEF_STRUCT2( CV, I, pC);
DEF_STRUCT2( DV, I, pD);

DEF_FUNTYPE1( V, pE);
DEF_FUNTYPE1( V, pCV);
DEF_FUNTYPE1( V, pDV);
DEF_FUNTYPE3( V, pE, I, pRV);
DEF_FUNTYPE3( V, pCV, I, C);
DEF_FUNTYPE3( V, pDV, I, D);
DEF_FUNTYPE3( V, pE, pV, pRV);

DEF_FUNTYPE1( I, pRV);
DEF_FUNTYPE1( I, pCV);
DEF_FUNTYPE1( I, pDV);

DEF_FUNTYPE2( C, pDV, I);
DEF_FUNTYPE2( C, pCV, I);

DEF_FUNTYPE1( pE, pE);
DEF_FUNTYPE2( pE, pE, pV);

DEF_FUNTYPE1( pRV, pDV);
DEF_FUNTYPE1( pRV, pCV);
DEF_FUNTYPE1( pRV, pF);
DEF_FUNTYPE1( pRV, pRV);
DEF_FUNTYPE2( pRV, pRV, pRV);
DEF_FUNTYPE2( pRV, pE, I);

DEF_FUNTYPE2( pF, pE, pV);
DEF_FUNTYPE1( pF, pRV);

DEF_FUNTYPE1( pCV, I);
DEF_FUNTYPE1( pCV, pRV);
DEF_FUNTYPE1( pCV, varC);
DEF_FUNTYPE2( pCV, pCV, pCV);

DEF_FUNTYPE1( pDV, I);
DEF_FUNTYPE1( pDV, varD);
DEF_FUNTYPE1( pDV, pRV);
DEF_FUNTYPE2( pDV, pDV, pDV);

///

DEF_FUNCTION( r_env_mk,     funtype_pE__pE);
DEF_FUNCTION( r_env_get,    funtype_pE__pE_pV);
DEF_FUNCTION( r_env_def,    funtype_pE__pE_pV);
DEF_FUNCTION( r_env_set,    funtype_V__pE_pV_pRV);
DEF_FUNCTION( r_env_set_at, funtype_V__pE_I_pRV);
DEF_FUNCTION( r_env_get_at, funtype_pRV__pE_I);
DEF_FUNCTION( r_env_del,    funtype_V__pE);

DEF_FUNCTION( r_fun_mk,     funtype_pF__pE_pV);

DEF_FUNCTION( r_cv_mk,      funtype_pCV__I);
DEF_FUNCTION( r_cv_c,       funtype_pCV__varC);
DEF_FUNCTION( r_cv_del,     funtype_V__pCV);
DEF_FUNCTION( r_cv_set,     funtype_V__pCV_I_C);
DEF_FUNCTION( r_cv_get,     funtype_C__pCV_I);
DEF_FUNCTION( r_cv_size,    funtype_I__pCV);
DEF_FUNCTION( r_cv_paste,   funtype_pCV__pCV_pCV);

DEF_FUNCTION( r_dv_mk,      funtype_pDV__I);
DEF_FUNCTION( r_dv_c,       funtype_pDV__varD);
DEF_FUNCTION( r_dv_del,     funtype_V__pDV);
DEF_FUNCTION( r_dv_set,     funtype_V__pDV_I_D);
DEF_FUNCTION( r_dv_get,     funtype_C__pDV_I);
DEF_FUNCTION( r_dv_size,    funtype_I__pDV);
DEF_FUNCTION( r_dv_paste,   funtype_pDV__pDV_pDV);

DEF_FUNCTION( r_rv_mk_dv,   funtype_pRV__pDV);
DEF_FUNCTION( r_rv_mk_cv,   funtype_pRV__pCV);
DEF_FUNCTION( r_rv_mk_fun,  funtype_pRV__pF);

DEF_FUNCTION( r_rv_as_dv,   funtype_pDV__pRV);
DEF_FUNCTION( r_rv_as_cv,   funtype_pCV__pRV);
DEF_FUNCTION( r_rv_as_fun,  funtype_pF__pRV);

DEF_FUNCTION( op_plus,       funtype_pRV__pRV_pRV);
DEF_FUNCTION( op_minus,      funtype_pRV__pRV_pRV);
DEF_FUNCTION( op_times,      funtype_pRV__pRV_pRV);
DEF_FUNCTION( op_divide,     funtype_pRV__pRV_pRV);

DEF_FUNCTION( isa_fun,       funtype_I__pRV);
DEF_FUNCTION( isa_dv,        funtype_I__pRV);
DEF_FUNCTION( isa_cv,        funtype_I__pRV);

DEF_FUNCTION( print,         funtype_pRV__pRV);
DEF_FUNCTION( paste,         funtype_pRV__pRV_pRV);
DEF_FUNCTION( eval,          funtype_pRV__pRV);


Type * initializeTypes() {
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
}


Module * initializeModule(std::string const & name) {
    Module * m = new Module(name, getGlobalContext());
    
    init_function_r_env_mk(m);    
    init_function_r_env_get(m);   
    init_function_r_env_def(m);   
    init_function_r_env_set(m);   
    init_function_r_env_set_at(m);
    init_function_r_env_get_at(m);
    init_function_r_env_del(m);  

    init_function_r_fun_mk(m);   

    init_function_r_cv_mk(m);    
    init_function_r_cv_c(m);     
    init_function_r_cv_del(m);   
    init_function_r_cv_set(m);   
    init_function_r_cv_get(m);   
    init_function_r_cv_size(m);  
    init_function_r_cv_paste(m); 
    
    init_function_r_dv_mk(m);    
    init_function_r_dv_c(m);     
    init_function_r_dv_del(m);   
    init_function_r_dv_set(m);   
    init_function_r_dv_get(m);   
    init_function_r_dv_size(m);
    init_function_r_dv_paste(m); 

    init_function_r_rv_mk_dv(m); 
    init_function_r_rv_mk_cv(m); 
    init_function_r_rv_mk_fun(m);
    
    init_function_r_rv_as_dv(m); 
    init_function_r_rv_as_cv(m); 
    init_function_r_rv_as_fun(m);

    init_function_op_plus(m);    
    init_function_op_minus(m);   
    init_function_op_times(m);   
    init_function_op_divide(m);  

    init_function_isa_fun(m);    
    init_function_isa_dv(m);     
    init_function_isa_cv(m);     
    init_function_print(m);       
    init_function_paste(m);       
    init_function_eval(m);        
    return m;
}

#define ARGS(...) std::vector<Value *>({ __VA_ARGS__ })

class Compiler : public Visitor {
  Module * m;
  BasicBlock * bb;
  Value* result;

  /** Numeric constant is converted to a vector of size 1. Better way
      would be to have a single API call for it, but this nicely shows how
      to actually work with the calls.
  */
  void visit(Num const & e) {
    // create a vector of size 1
    result = CallInst::Create(f_createDV, ARGS(constant(1)), "", bb);
    // set its first element to the given double
    CallInst::Create(f_setDVElem, ARGS(result, constant(0), constant(e.value)), "", bb);
  }

  /** Compiles a call to c() function.  */
  void visit(Call const & e) {
    std::vector<Value *> args;
    args.push_back(constant(static_cast<int>(e.args->size()))); 
    // number of arguments
    for (Exp * arg : *e.args)
      args.push_back(arg->codegen()); 
    // this will not be codegen but a visitor pattern
    result = CallInst::Create(f_concatDV, args, "", bb);
  }

  void constant(int value) {
    result = ConstantInt::get(getGlobalContext(), APInt(value, 32));
  }

  void constant(double value) {
    result = ConstantFP::get(getGlobalContext(), APFloat(value));
  }
};


} // namespace

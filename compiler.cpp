#include <llvm/IR/Verifier.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/Support/raw_ostream.h>
#include "llvm/Analysis/Passes.h"

#include "parse.h"

using namespace llvm;
using namespace rift;

namespace {


class RiftModule : public Module {

#define DEF_BASE_TYPE( name , code ) \
  Type *init_ ## name() { return code; }  \
  Type * name = init_ ## name();

#define DEF_PTR_TYPE( name , code ) \
  PointerType *init_ ## name() { return code; }  \
  PointerType * name = init_ ## name();

#define DEF_OPAQUE_PTR_TYPE( name ) \
  PointerType *init_opaque_ ## name() { \
  llvm::PATypeHolder opaqueStructTy = OpaqueType::get();	\
     return PointerType::getUnqual(opaqueStructTy); \
  }; \
  PointerType * name = init_opaque_ ## name ();

#define DEF_FUNCTION( name, signature ) \
  Function *init_function_ ## name (Module *module) {		\
    return Function::Create( signature, Function::ExternalLinkage,\
                             #name, module); \
  } \
  Function *fun_ ## name = init_function_ ## name ( this );

#define DEF_FUNTYPEV( returntype , argtype )	\
  FunctionType*  init_funtype_ ## returntype ## __ ## argtype ##_v () { \
     std::vector<Type *> args; \
     args = { argtype }; \
     return FunctionType::get( returntype, args, true ); \
  } \
  FunctionType * funtype_ ## returntype ## __ ## argtype ## _v = 	\
    init_funtype_ ## returntype ## __ ## argtype ## _v();

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
 
#define DEF_STRUCT0( name  )	\
  StructType * name = StructType::create(getGlobalContext(), #name);
 
#define DEF_STRUCT2( name , a1, a2 )	\
  StructType * init_ ## name () { \
    StructType * t = StructType::create(getGlobalContext(), #name);\
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

//How to deal with recursive types in LLVM 3+
//http://blog.llvm.org/2011/11/llvm-30-type-system-rewrite.html

#define FILL_STRUCT2( name , a1, a2 )	\
  int init_ ## name () { \
    std::vector<Type *> fields; \
    fields.push_back(a1);\
    fields.push_back(a2);\
    name->setBody(fields, false);\
    return 0;\
  } \
  int _IGNORE_ ## name = init_ ## name();

#define FILL_STRUCT4( name , a1, a2, a3, a4 )	\
  int init_ ## name () { \
    std::vector<Type *> fields; \
    fields.push_back(a1);\
    fields.push_back(a2);\
    fields.push_back(a3);\
    fields.push_back(a4);\
    name->setBody(fields, false);\
    return 0;\
  } \
  int _IGNORE_ ## name = init_ ## name();

public:
  DEF_BASE_TYPE( V,  Type::getVoidTy(getGlobalContext()));
  DEF_BASE_TYPE( I,  IntegerType::get(getGlobalContext(), 32));
  DEF_BASE_TYPE( D,  Type::getDoubleTy(getGlobalContext()));
  DEF_BASE_TYPE( C, IntegerType::get(getGlobalContext(), 8));


  /* LLVM does not have void* type, changed to int. */
  DEF_PTR_TYPE( pI, PointerType::get(I, 0));
  DEF_PTR_TYPE( pC, PointerType::get(C, 0));
  DEF_PTR_TYPE( pD, PointerType::get(D, 0));

  DEF_STRUCT2( CV, I, pC);
  DEF_STRUCT2( DV, I, pD);

  DEF_PTR_TYPE( pCV, PointerType::get(CV, 0) );
  DEF_PTR_TYPE( pDV, PointerType::get(DV, 0) );

  DEF_STRUCT0( RV);
  DEF_STRUCT0( B);
  DEF_STRUCT0( E);
  DEF_STRUCT0( F);

  DEF_PTR_TYPE( pE, PointerType::get(E, 0));
  DEF_PTR_TYPE( pRV, PointerType::get(RV, 0) );
  DEF_PTR_TYPE( pF, PointerType::get(F, 0) );
  DEF_PTR_TYPE( pB, PointerType::get(B, 0) );

  FILL_STRUCT2( RV, 
	       I,  // this is an enum.
	       F); // this a union, F is likely the largest

  FILL_STRUCT2( B, pI, pRV);
  FILL_STRUCT4( E, pE, pB, I, I);
  FILL_STRUCT2( F, pE, pI);

  DEF_FUNTYPE1( V, pE);
  DEF_FUNTYPE1( V, pCV);
  DEF_FUNTYPE1( V, pDV);
  DEF_FUNTYPE3( V, pE, I, pRV);
  DEF_FUNTYPE3( V, pCV, I, C);
  DEF_FUNTYPE3( V, pDV, I, D);
  DEF_FUNTYPE3( V, pE, pI, pRV);

  DEF_FUNTYPE1( I, pRV);
  DEF_FUNTYPE1( I, pCV);
  DEF_FUNTYPE1( I, pDV);

  DEF_FUNTYPE2( C, pDV, I);
  DEF_FUNTYPE2( C, pCV, I);

  DEF_FUNTYPE1( pE, pE);
  DEF_FUNTYPE2( pE, pE, pI);

  DEF_FUNTYPE1( pRV, pDV);
  DEF_FUNTYPE1( pRV, pCV);
  DEF_FUNTYPE1( pRV, pF);
  DEF_FUNTYPE1( pRV, pRV);
  DEF_FUNTYPE2( pRV, pRV, pRV);
  DEF_FUNTYPE2( pRV, pE, I);

  DEF_FUNTYPE1( pRV, pE);

  DEF_FUNTYPE2( pF, pE, pI);
  DEF_FUNTYPE1( pF, pRV);

  DEF_FUNTYPE1( pCV, I);
  DEF_FUNTYPE1( pCV, pC);
  DEF_FUNTYPE1( pCV, pRV);
  DEF_FUNTYPE2( pCV, pCV, pCV);

  DEF_FUNTYPE1( pDV, I);
  DEF_FUNTYPE1( pDV, pRV);
  DEF_FUNTYPE2( pDV, pDV, pDV);

  DEF_FUNTYPEV( pCV, C);
  DEF_FUNTYPEV( pDV, D);

  DEF_FUNCTION( r_env_mk,     funtype_pE__pE);
  DEF_FUNCTION( r_env_get,    funtype_pE__pE_pI);
  DEF_FUNCTION( r_env_def,    funtype_pE__pE_pI);
  DEF_FUNCTION( r_env_set,    funtype_V__pE_pI_pRV);
  DEF_FUNCTION( r_env_set_at, funtype_V__pE_I_pRV);
  DEF_FUNCTION( r_env_get_at, funtype_pRV__pE_I);
  DEF_FUNCTION( r_env_del,    funtype_V__pE);

  DEF_FUNCTION( r_fun_mk,     funtype_pF__pE_pI);

  DEF_FUNCTION( r_cv_mk,      funtype_pCV__I);
  DEF_FUNCTION( r_cv_mk_from_char, 
		              funtype_pCV__pC);
  DEF_FUNCTION( r_cv_c,       funtype_pCV__C_v);
  DEF_FUNCTION( r_cv_del,     funtype_V__pCV);
  DEF_FUNCTION( r_cv_set,     funtype_V__pCV_I_C);
  DEF_FUNCTION( r_cv_get,     funtype_C__pCV_I);
  DEF_FUNCTION( r_cv_size,    funtype_I__pCV);
  DEF_FUNCTION( r_cv_paste,   funtype_pCV__pCV_pCV);

  DEF_FUNCTION( r_dv_mk,      funtype_pDV__I);
  DEF_FUNCTION( r_dv_c,       funtype_pDV__D_v);
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

  RiftModule(std::string const & name) : Module( name, getGlobalContext()) {
  }
};    

#define ARGS(...) std::vector<Value *>({ __VA_ARGS__ })

class Compiler : public Visitor {
public:
    Compiler(std::string const & moduleName):
        m(new RiftModule(moduleName)) {}

    Function * compileFunction(Fun * function) {
        f = Function::Create(m->funtype_pRV__pE, Function::ExternalLinkage, "riftFunction", m);
        bb = BasicBlock::Create(gc, "functionStart", f, nullptr);
        // compile the function's body
        function->body->accept(this);
        //Value * r = new BitCastInst(result, m->pRV, "", bb);
        ReturnInst::Create(gc, result, bb);
        f->dump();
        return f;
    }



    Function * f;
  BasicBlock  *bb;
  Value       *result;
  RiftModule  *m;
  LLVMContext & gc = getGlobalContext();

  Value *r_const(int value) {
    return ConstantInt::get(gc, APInt(32, value));
  }

  Value *r_const(double value) {
    return ConstantFP::get(gc, APFloat(value));
  }

  Value *r_const(string const & value) {
      ArrayType * arrayType = ArrayType::get(IntegerType::get(gc, 8), value.size() + 1);
      GlobalVariable * gvar = new GlobalVariable(*m, arrayType, true, GlobalVariable::PrivateLinkage, 0, value);
      Constant * cstr = ConstantDataArray::getString(gc, value.c_str(), true);
      gvar->setInitializer(cstr);
      std::vector<Value*> indices({ r_const(0), r_const(0) });
      return ConstantExpr::getGetElementPtr(gvar, indices);
  }

  /** Numeric constant is converted to a vector of size 1. Better way
      would be to have a single API call for it, but this nicely shows how
      to actually work with the calls.
  */
  void visit(Num *e) {
    result = CallInst::Create(m->fun_r_dv_mk, ARGS(r_const(1)), "", bb);
    CallInst::Create(m->fun_r_dv_set,
		     ARGS(result, r_const(0), r_const(e->value)), 
		     "", 
		     bb);
    result = CallInst::Create(m->fun_r_rv_mk_dv, ARGS(result), "", bb);
  }

  /** Compiles a call to c() function.  */
  void visit(Call *e) {
    std::vector<Value *> args;
    args.push_back(r_const(static_cast<int>(e->args->size()))); 
    for (Exp * arg : *e->args) {
      arg->accept(this);
      args.push_back(result);
    }
    result = CallInst::Create(m->fun_r_dv_c, args, "", bb);
  }


  void visit(Str * x)  {
    result = CallInst::Create(m->fun_r_cv_mk_from_char, 
                  ARGS(r_const(*(x->value))), "", bb);
    result = CallInst::Create(m->fun_r_rv_mk_cv, ARGS(result), "", bb);
  }
   void visit(Var * x)  {}
   void visit(Fun * x)  {}

   void visit(BinExp * x) {
       x->left->accept(this);
       Value * lhs = result;
       x->right->accept(this);
       Value * rhs = result;
       Function * target = nullptr;
       switch (x->op) {
       case Token::PLUS:
           target = m->fun_op_plus;
           break;
       case Token::MIN:
           target = m->fun_op_minus;
           break;
       case Token::MUL:
           target = m->fun_op_times;
           break;
       case Token::DIV:
           target = m->fun_op_divide;
       }
       result = CallInst::Create(target, ARGS(lhs, rhs), "", bb);
   }

   void visit(Seq * x) {
       for (Exp * e : *(x->exps))
           e->accept(this);
   }

   void visit(Idx * x)    {} 
   void visit(SimpleAssign * x) {}
   void visit(IdxAssign * x) {}
   void visit(IfElse * x) {}

};


} // namespace


void * test_compileFunction(Fun * f) {
    Compiler c("test");
    Function * result = c.compileFunction(f);
    // now try to JIT it
    ExecutionEngine * engine = EngineBuilder(std::unique_ptr<Module>(c.m)).create();
    engine->finalizeObject();
    return engine->getPointerToFunction(result);
}


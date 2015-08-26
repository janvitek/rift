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


struct RVal;
struct Env;

typedef RVal * (*FunPtr)(Env *);

struct FunInfo {
    FunPtr ptr;
    std::vector<int> args;
};

extern std::vector<FunInfo> registeredFunctions;
extern std::unordered_map<std::string, int> symbols;

int registerSymbol(char const * c) {
    int result = symbols.size();
    symbols[c] = result;
    return result;
}

namespace symbol {
    int const c = registerSymbol("c");
    int const eval = registerSymbol("eval");


}


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

#define DEF_FUNTYPEV2( returntype , argtype, argtype2 )	\
  FunctionType*  init_funtype_ ## returntype ## __ ## argtype ## _ ## argtype2 ##_v () { \
     std::vector<Type *> args; \
     args = { argtype, argtype2 }; \
     return FunctionType::get( returntype, args, true ); \
  } \
  FunctionType * funtype_ ## returntype ## __ ## argtype ## _ ## argtype2 ## _v = 	\
    init_funtype_ ## returntype ## __ ## argtype ## _ ## argtype2 ## _v();

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
  DEF_BASE_TYPE( V, Type::getVoidTy(getGlobalContext()));
  DEF_BASE_TYPE( I, IntegerType::get(getGlobalContext(), 32));
  DEF_BASE_TYPE( D, Type::getDoubleTy(getGlobalContext()));
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
	       I,  // an enum.
	       F); // a union, F is the largest

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
  DEF_FUNTYPE2( pE, pE, I);

  DEF_FUNTYPE1( pRV, pDV);
  DEF_FUNTYPE1( pRV, pCV);
  DEF_FUNTYPE1( pRV, pF);
  DEF_FUNTYPE1( pRV, pRV);
  DEF_FUNTYPE2( pRV, pRV, pRV);
  DEF_FUNTYPE3( pRV, pRV, pRV, pRV);
  DEF_FUNTYPE2( pRV, pE, I);

  DEF_FUNTYPE1( pRV, pE);
  DEF_FUNTYPE2( pRV, pE, pRV);

  DEF_FUNTYPE2( pF, pE, pI);
  DEF_FUNTYPE2( pF, pE, I);
  DEF_FUNTYPE1( pF, pRV);

  DEF_FUNTYPE1( pCV, I);
  DEF_FUNTYPE1( pCV, pC);
  DEF_FUNTYPE1( pCV, pRV);
  DEF_FUNTYPE2( pCV, pCV, pCV);

  DEF_FUNTYPE1( pDV, I);
  DEF_FUNTYPE1( pDV, pRV);
  DEF_FUNTYPE2( pDV, pDV, pDV);
  DEF_FUNTYPE2( V, pE, I)

  DEF_FUNTYPEV( pCV, C);
  DEF_FUNTYPEV( pDV, D);
  DEF_FUNTYPEV( pRV, I);
  DEF_FUNTYPEV2( pRV, pRV, I);

  DEF_FUNCTION( r_env_mk,     funtype_pE__pE);
  DEF_FUNCTION( r_env_get,    funtype_pRV__pE_I);
  DEF_FUNCTION( r_env_def,    funtype_V__pE_I);
  DEF_FUNCTION( r_env_set,    funtype_V__pE_I_pRV);
  DEF_FUNCTION( r_env_set_at, funtype_V__pE_I_pRV);
  DEF_FUNCTION( r_env_get_at, funtype_pRV__pE_I);
  DEF_FUNCTION( r_env_del,    funtype_V__pE);

  DEF_FUNCTION( r_fun_mk,     funtype_pF__pE_I);

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
  DEF_FUNCTION( eval,          funtype_pRV__pE_pRV);

  DEF_FUNCTION( r_call, funtype_pRV__pRV_I_v);
  DEF_FUNCTION( r_c, funtype_pRV__I_v);

  DEF_FUNCTION( r_getIndex, funtype_pRV__pRV_pRV);
  DEF_FUNCTION( r_setIndex, funtype_pRV__pRV_pRV_pRV);

  DEF_FUNCTION( r_guard, funtype_I__pRV);

  RiftModule(std::string const & name) : Module( name, getGlobalContext()) {
  }
};    

#define ARGS(...) std::vector<Value *>({ __VA_ARGS__ })



class Compiler : public Visitor {
public:
    Compiler(std::string const & moduleName):
        m(new RiftModule(moduleName)) {}

    FunPtr compileAndJIT(Fun * function) {
        int start = registeredFunctions.size();
        int result = compileFunction(function);
        ExecutionEngine * engine = EngineBuilder(std::unique_ptr<Module>(m)).create();
        engine->finalizeObject();
        for (; start < registeredFunctions.size(); ++start) {
            FunInfo & i = registeredFunctions[start];
            i.ptr = reinterpret_cast<FunPtr>(engine->getPointerToFunction(reinterpret_cast<Function*>(i.ptr)));
        }
        return registeredFunctions[result].ptr;
    }


    int compileFunction(Fun * function) {
        Function * oldf = f;
        BasicBlock * oldbb = bb;
        Value * oldenv = env;
        f = Function::Create(m->funtype_pRV__pE, Function::ExternalLinkage, "riftFunction", m);
        bb = BasicBlock::Create(gc, "functionStart", f, nullptr);
        Function::arg_iterator args = f->arg_begin();
        env = args++;
        env->setName("env");
        // compile the function's body
        function->body->accept(this);
        //Value * r = new BitCastInst(result, m->pRV, "", bb);
        ReturnInst::Create(gc, result, bb);
        f->dump();
        FunInfo fi;
        fi.ptr = reinterpret_cast<FunPtr>(f);
        for (Var * v : function->params)
            fi.args.push_back(v->index);
        registeredFunctions.push_back(fi);
        f = oldf;
        bb = oldbb;
        env = oldenv;
        return registeredFunctions.size() - 1;
    }


    Value * env;
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

  Value * r_const(void * symbol) {

  }

  Value *r_const(std::string const & value) {
    ArrayType * arrayType = ArrayType::get(IntegerType::get(gc, 8), 
					   value.size() + 1);
      GlobalVariable * gvar = 
	new GlobalVariable(*m, arrayType, true, 
			   GlobalVariable::PrivateLinkage, 0, value);
      Constant * cstr = ConstantDataArray::getString(gc, value.c_str(), true);
      gvar->setInitializer(cstr);
      std::vector<Value*> indices({ r_const(0), r_const(0) });
      return ConstantExpr::getGetElementPtr(gvar, indices);
  }

  /** Numeric constant is converted to a vector of size 1. */
  void visit(Num *e) {
    result = CallInst::Create(m->fun_r_dv_mk, ARGS(r_const(1)), "", bb);
    CallInst::Create(m->fun_r_dv_set,
		     ARGS(result, r_const(0), r_const(e->value)), 
		     "", 
		     bb);
    result = CallInst::Create(m->fun_r_rv_mk_dv, ARGS(result), "", bb);
  }

  /** Compiles a call to a function.  */
  void visit(Call *e) {
      if (e->name->index == symbol::c) {
          call_c(e);
      } else if (e->name->index == symbol::eval) {
          call_eval(e);
      } else {
          e->name->accept(this);
          std::vector<Value *> args;
          args.push_back(result);
          args.push_back(r_const(static_cast<int>(e->args.size())));
          for (Exp * arg : e->args) {
            arg->accept(this);
            args.push_back(result);
          }
          result = CallInst::Create(m->fun_r_call, args, "", bb);
      }

      //result = CallInst::Create(m->fun_r_dv_c, args, "", bb);
  }

  void call_c(Call * e) {
      std::vector<Value *> args;
      args.push_back(r_const(static_cast<int>(e->args.size())));
      for (Exp * arg : e->args) {
          arg->accept(this);
          args.push_back(result);
      }
      result = CallInst::Create(m->fun_r_c, args, "", bb);
  }

  void call_eval(Call * e) {
      assert(e->args.size() == 1 and "Only one argument to eval allowed");
      e->args[0]->accept(this);
      result = CallInst::Create(m->fun_eval, ARGS(env, result), "", bb);
  }


  void visit(Str * x)  {
    result = CallInst::Create(m->fun_r_cv_mk_from_char, 
                  ARGS(r_const(x->value)), "", bb);
    result = CallInst::Create(m->fun_r_rv_mk_cv, ARGS(result), "", bb);
  }
   void visit(Var * x)  {
       result = CallInst::Create(m->fun_r_env_get, ARGS(env, r_const(x->index)), "", bb);

   }

   void visit(Fun * x)  {
       int fi = compileFunction(x);
       result = CallInst::Create(m->fun_r_fun_mk, ARGS(env, r_const(fi)), "", bb);
       result = CallInst::Create(m->fun_r_rv_mk_fun, ARGS(result), "", bb);
   }

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
       if (x->exps.size() == 0) {
           // return 0 if the sequence is empty
           result = CallInst::Create(m->fun_r_dv_mk, ARGS(r_const(1)), "", bb);
           CallInst::Create(m->fun_r_dv_set,
                    ARGS(result, r_const(0), r_const(0)),
                    "",
                    bb);
           result = CallInst::Create(m->fun_r_rv_mk_dv, ARGS(result), "", bb);
       }
       for (Exp * e : x->exps)
           e->accept(this);
   }

   void visit(Idx * x)    {
       x->name->accept(this);
       Value * source = result;
       x->body->accept(this);
       result = CallInst::Create(m->fun_r_getIndex, ARGS(source, result), "", bb);
   }

   void visit(SimpleAssign * x) {
       x->rhs->accept(this);
       CallInst::Create(m->fun_r_env_set, ARGS(env, r_const(x->name->index), result), "", bb);
   }
   void visit(IdxAssign * x) {
       x->lhs->name->accept(this);
       Value * target = result;
       x->lhs->body->accept(this);
       Value * index = result;
       x->rhs->accept(this);
       result = CallInst::Create(m->fun_r_setIndex, ARGS(target, index, result), "", bb);
   }

   void visit(IfElse * x) {
       x->guard->accept(this);
       Value * cond = CallInst::Create(m->fun_r_guard, ARGS(result), "", bb);
       BasicBlock * ifTrue = BasicBlock::Create(getGlobalContext(), "ifTrue", f, nullptr);
       BasicBlock * ifFalse = BasicBlock::Create(getGlobalContext(), "ifFalse", f, nullptr);
       BasicBlock * next = BasicBlock::Create(getGlobalContext(), "next", f, nullptr);
       ICmpInst * test = new ICmpInst(*bb, ICmpInst::ICMP_EQ, cond, r_const(1), "condition");
       BranchInst::Create(ifTrue, ifFalse, test, bb);
       bb = ifTrue;
       x->ifclause->accept(this);
       BranchInst::Create(next,bb);
       Value * trueResult = result;
       bb = ifFalse;
       x->elseclause->accept(this);
       BranchInst::Create(next,bb);
       Value * falseResult = result;
       bb = next;
       PHINode * phi = PHINode::Create(m->pRV, 2, "", bb);
       phi->addIncoming(trueResult, ifTrue);
       phi->addIncoming(falseResult, ifFalse);
       result = phi;
   }
};


} // namespace


FunPtr test_compileFunction(Fun * f) {
    Compiler c("test");
    return c.compileAndJIT(f);
}


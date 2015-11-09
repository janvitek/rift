#include "parser.h"
#include "runtime.h"
#include "compiler.h"
#include "type_analysis.h"
#include "unboxing.h"
#include "boxing_removal.h"
#include "pool.h"

#include <initializer_list>
using namespace llvm;

namespace rift {

namespace type {

    /** Initialization of the type declarations.

    Each C/C++ type we are using must be declared here in LLVM structures so that LLVM understands it. 
     */
#define STRUCT(name, ...) StructType::create(name, __VA_ARGS__, nullptr)
#define FUN_TYPE(result, ...) FunctionType::get(result, std::vector<llvm::Type*>({ __VA_ARGS__}), false)
#define FUN_TYPE_VARARG(result, ...) FunctionType::get(result, std::vector<llvm::Type*>({ __VA_ARGS__}), true)

    StructType * environmentType();

    llvm::Type * Void = llvm::Type::getVoidTy(getGlobalContext());
    llvm::Type * Int = IntegerType::get(getGlobalContext(), 32);
    llvm::Type * Double = llvm::Type::getDoubleTy(getGlobalContext());
    llvm::Type * Character = IntegerType::get(getGlobalContext(), 8);
    llvm::Type * Bool = IntegerType::get(getGlobalContext(), 1);

    PointerType * ptrInt = PointerType::get(Int, 0);
    PointerType * ptrCharacter = PointerType::get(Character, 0);
    PointerType * ptrDouble = PointerType::get(Double, 0);

    StructType * DoubleVector = STRUCT("DoubleVector", ptrDouble, Int);
    StructType * CharacterVector = STRUCT("CharacterVector", ptrCharacter, Int);
    PointerType * ptrDoubleVector = PointerType::get(DoubleVector, 0);
    PointerType * ptrCharacterVector = PointerType::get(CharacterVector, 0);

    // union
    StructType * Value = STRUCT("Value", Int, ptrDoubleVector);
    PointerType * ptrValue = PointerType::get(Value, 0);


    StructType * Binding = STRUCT("Binding", Int, ptrValue);
    PointerType * ptrBinding = PointerType::get(Binding, 0);

    PointerType * ptrEnvironment;
    StructType * Environment = environmentType();

    FunctionType * NativeCode = FUN_TYPE(ptrValue, ptrEnvironment);

    StructType * Function = STRUCT("Function", ptrEnvironment, NativeCode, ptrInt, Int);
    PointerType * ptrFunction = PointerType::get(Function, 0);


    FunctionType * dv_d = FUN_TYPE(ptrDoubleVector, Double);
    FunctionType * cv_i = FUN_TYPE(ptrCharacterVector, Int);
    FunctionType * v_dv = FUN_TYPE(ptrValue, ptrDoubleVector);
    FunctionType * v_cv = FUN_TYPE(ptrValue, ptrCharacterVector);
    FunctionType * v_ev = FUN_TYPE(ptrValue, ptrEnvironment, ptrValue);
    FunctionType * v_vv = FUN_TYPE(ptrValue, ptrValue, ptrValue);
    FunctionType * v_vvv = FUN_TYPE(ptrValue, ptrValue, ptrValue, ptrValue);
    FunctionType * v_vi = FUN_TYPE(ptrValue, ptrValue, Int);
    FunctionType * v_viv = FUN_TYPE(ptrValue, ptrValue, Int, ptrValue);
    FunctionType * v_ei = FUN_TYPE(ptrValue, ptrEnvironment, Int);
    FunctionType * void_eiv = FUN_TYPE(Void, ptrEnvironment, Int, ptrValue);
    FunctionType * dv_dvdv = FUN_TYPE(ptrDoubleVector, ptrDoubleVector, ptrDoubleVector);
    FunctionType * cv_cvcv = FUN_TYPE(ptrCharacterVector, ptrCharacterVector, ptrCharacterVector);
    FunctionType * dv_cvcv = FUN_TYPE(ptrDoubleVector, ptrCharacterVector, ptrCharacterVector);
    FunctionType * d_dvd = FUN_TYPE(Double, ptrDoubleVector, Double);
    FunctionType * cv_cvdv = FUN_TYPE(ptrCharacterVector, ptrCharacterVector, ptrDoubleVector);

    FunctionType * v_f = FUN_TYPE(ptrValue, ptrFunction);
    FunctionType * f_ie = FUN_TYPE(ptrFunction, Int, ptrEnvironment);

    FunctionType * b_v = FUN_TYPE(Bool, ptrValue);

    FunctionType * v_viVA = FUN_TYPE_VARARG(ptrValue, ptrValue, Int);

    FunctionType * void_vvv = FUN_TYPE(Void, ptrValue, ptrValue, ptrValue);
    FunctionType * void_dvdvdv = FUN_TYPE(Void, ptrDoubleVector, ptrDoubleVector, ptrDoubleVector);
    FunctionType * void_cvdvcv = FUN_TYPE(Void, ptrCharacterVector, ptrDoubleVector, ptrCharacterVector);
    FunctionType * void_dvdd = FUN_TYPE(Void, ptrDoubleVector, Double, Double);

    FunctionType * d_v = FUN_TYPE(Double, ptrValue);
    FunctionType * cv_v = FUN_TYPE(ptrCharacterVector, ptrValue);
    FunctionType * v_iVA = FUN_TYPE_VARARG(ptrValue, Int);
    FunctionType * dv_iVA = FUN_TYPE_VARARG(ptrDoubleVector, Int);
    FunctionType * cv_iVA = FUN_TYPE_VARARG(ptrCharacterVector, Int);

    FunctionType * dv_v = FUN_TYPE(ptrDoubleVector, ptrValue);
    FunctionType * d_dv = FUN_TYPE(Double, ptrDoubleVector);
    FunctionType * f_v = FUN_TYPE(ptrFunction, ptrValue);


    StructType * environmentType() {
        StructType * result = StructType::create(getGlobalContext(), "Environment");
        ptrEnvironment = PointerType::get(result, 0);
        result->setBody(ptrEnvironment, ptrBinding, Int, nullptr);
        return result;
    }

} // namespace rift::type

/** Memory manager helps with loading symbols of runtime functions. 
 */
class MemoryManager : public llvm::SectionMemoryManager {
    MemoryManager(const MemoryManager&) = delete;
    void operator=(const MemoryManager&) = delete;

public:
    MemoryManager() = default;
    virtual ~MemoryManager() {}

#define CHECK_RUNTIME(name) if (Name == #name) return reinterpret_cast<uint64_t>(::name)
    /** Given a string name, returns the symbol corresponding to it, nullptr if no such symbol exists. 
    
    The purpose of this function is to aid LLVM's runtime symbol resolving, which differs between platforms. Thus in a rather trivial way, we check for our own runtime symbols and return pointers to their respective functions if applicable. 
    */
    uint64_t getSymbolAddress(const std::string &Name) override {
        uint64_t addr = SectionMemoryManager::getSymbolAddress(Name);
        if (addr) {
            return addr;
        } else {
            CHECK_RUNTIME(envCreate);
            CHECK_RUNTIME(envGet);
            CHECK_RUNTIME(envSet);
            CHECK_RUNTIME(doubleVectorLiteral);
            CHECK_RUNTIME(characterVectorLiteral);
            CHECK_RUNTIME(fromDoubleVector);
            CHECK_RUNTIME(fromCharacterVector);
            CHECK_RUNTIME(fromFunction);
            CHECK_RUNTIME(doubleFromValue);
            CHECK_RUNTIME(scalarFromVector);
            CHECK_RUNTIME(characterFromValue);
            CHECK_RUNTIME(functionFromValue);
            CHECK_RUNTIME(doubleGetSingleElement);
            CHECK_RUNTIME(doubleGetElement);
            CHECK_RUNTIME(characterGetElement);
            CHECK_RUNTIME(genericGetElement);
            CHECK_RUNTIME(doubleSetElement);
            CHECK_RUNTIME(scalarSetElement);
            CHECK_RUNTIME(characterSetElement);
            CHECK_RUNTIME(genericSetElement);
            CHECK_RUNTIME(doubleAdd);
            CHECK_RUNTIME(characterAdd);
            CHECK_RUNTIME(genericAdd);
            CHECK_RUNTIME(doubleSub);
            CHECK_RUNTIME(genericSub);
            CHECK_RUNTIME(doubleMul);
            CHECK_RUNTIME(genericMul);
            CHECK_RUNTIME(doubleDiv);
            CHECK_RUNTIME(genericDiv);
            CHECK_RUNTIME(doubleEq);
            CHECK_RUNTIME(characterEq);
            CHECK_RUNTIME(genericEq);
            CHECK_RUNTIME(doubleNeq);
            CHECK_RUNTIME(characterNeq);
            CHECK_RUNTIME(genericNeq);
            CHECK_RUNTIME(doubleLt);
            CHECK_RUNTIME(genericLt);
            CHECK_RUNTIME(doubleGt);
            CHECK_RUNTIME(genericGt);
            CHECK_RUNTIME(createFunction);
            CHECK_RUNTIME(toBoolean);
            CHECK_RUNTIME(call);
            CHECK_RUNTIME(length);
            CHECK_RUNTIME(type);
            CHECK_RUNTIME(eval);
            CHECK_RUNTIME(characterEval);
            CHECK_RUNTIME(genericEval);
            CHECK_RUNTIME(doublec);
            CHECK_RUNTIME(characterc);
            CHECK_RUNTIME(c);
            report_fatal_error("Program used extern function '" + Name +
                "' which could not be resolved!");
        }
        return addr;

    }

};

/** The compiler: a visitor over the AST.
*/
class Compiler : public Visitor {
public:
    /** Creates the module to which the function will be compiled.
     */
    Compiler(): m(new RiftModule()) { }

  /** Compiles a function and returns a pointer to the native code.  JIT
      compilation in LLVM finalizes the module, this function can only be
      called once.
  */
  FunPtr compile(ast::Fun * what) {
    unsigned start = Pool::functionsCount();
    int result = compileFunction(what);
    ExecutionEngine * engine = EngineBuilder(std::unique_ptr<Module>(m))
      .setMCJITMemoryManager(std::unique_ptr<MemoryManager>(new MemoryManager())).create();
    optimizeModule(engine);
    engine->finalizeObject();
    // Compile newly registered functions; update their native code in the
    // registered functions vector
    for (; start < Pool::functionsCount(); ++start) {
      ::Function * rec = Pool::getFunction(start);
      rec->code = reinterpret_cast<FunPtr>(engine->getPointerToFunction(rec->bitcode));
    }
    return Pool::getFunction(result)->code;
  }

  /** Optimize on the bitcode before native code generation. The
      TypeAnalysis, Unboxing and BoxingRemoval are Rift passes, the rest is
      from LLVM.
   */
  void optimizeModule(ExecutionEngine * ee) {
    auto *pm = new legacy::FunctionPassManager(m);
    m->setDataLayout(* ee->getDataLayout());
    pm->add(new TypeAnalysis());
    pm->add(new Unboxing());
    pm->add(new BoxingRemoval());
    pm->add(createConstantPropagationPass());
    // Optimize each function of this module
    for (llvm::Function & f : *m) {
      pm->run(f);
    }
    delete pm;
    }

    /** Translates a function to bitcode, registers it with the runtime, and
	returns its index.
    */
    int compileFunction(ast::Fun * node) {
        // Backup context in case we are creating a nested function
        llvm::Function * oldF = f;
        BasicBlock * oldB = b;
        llvm::Value * oldEnv = env;
	// Create the function and its first BB
        f = llvm::Function::Create(type::NativeCode, 
				   llvm::Function::ExternalLinkage, 
				   "riftFunction", 
				   m);
        b = BasicBlock::Create(getGlobalContext(), 
			       "entry", 
			       f, 
			       nullptr);
        // Get the (single) argument of the function and store is as the
        // environment
        llvm::Function::arg_iterator args = f->arg_begin();
        env = args++;
        env->setName("env");
        // Compile body 
        node->body->accept(this);
        // Append return instruction of the last used value
        ReturnInst::Create(getGlobalContext(), result, b);

        // Register and get index
        int result = Pool::addFunction(node, f);
        // Restore context
        f = oldF;
        b = oldB;
        env = oldEnv;
        return result;
    }


    /** Shorthand for calling runtime functions.  */
#define RUNTIME_CALL(name, ...) \
  CallInst::Create(m->name, \
		   std::vector<llvm::Value*>({__VA_ARGS__}), \
		   "", \
		   b)


    /** Create Value from double scalar .  */
    llvm::Value * fromDouble(double value) {
        return ConstantFP::get(getGlobalContext(), APFloat(value));
    }

    /** Create Value from integer. */
    llvm::Value * fromInt(int value) {
        return ConstantInt::get(getGlobalContext(), APInt(32, value));
    }

  /** Safeguard against forgotten  visitor methods.   */
  void visit(ast::Exp * node) override {
    throw "Unexpected: You are missing a visit() method.";
  }

  /** Get the double value, box it into a vector of length 1, box that into
      a Rift Value.
   */
    void visit(ast::Num * node) override {
        result = RUNTIME_CALL(doubleVectorLiteral, fromDouble(node->value));
        result = RUNTIME_CALL(fromDoubleVector, result);
    }

    /** Similarly string is loaded as character vector and then boxed into value. 
     */
    void visit(ast::Str * node) override {
        result = RUNTIME_CALL(characterVectorLiteral, fromInt(node->index));
        result = RUNTIME_CALL(fromCharacterVector, result);
    }

    /** Variable translates into reading from environment. 
     */
    void visit(ast::Var * node) override {
        result = RUNTIME_CALL(envGet, env, fromInt(node->symbol));
    }

    /** Sequence is compilation of each of its elements. The last one will stay in the result. 
     */
    void visit(ast::Seq * node) override {
        for (ast::Exp * e : node->body)
            e->accept(this);
    }

  /** Function declaration.  Compiles function, use its id as a constant for
      createFunction() which binding the function code to the
      environment. Box result into a value.
  */
  void visit(ast::Fun * node) override {
    int fi = compileFunction(node);
    result = RUNTIME_CALL(createFunction, fromInt(fi), env);
    result = RUNTIME_CALL(fromFunction, result);
  }

  /** Binary expression first compiles its arguments and then calls
	respective runtime based on the type of the operation.
     */
    void visit(ast::BinExp * node) override {
        node->lhs->accept(this);
        llvm::Value * lhs = result;
        node->rhs->accept(this);
        llvm::Value * rhs = result;
        switch (node->type) {
        case ast::BinExp::Type::add:
            result = RUNTIME_CALL(genericAdd, lhs, rhs);
            return;
        case ast::BinExp::Type::sub:
            result = RUNTIME_CALL(genericSub, lhs, rhs);
            return;
        case ast::BinExp::Type::mul:
            result = RUNTIME_CALL(genericMul, lhs, rhs);
            return;
        case ast::BinExp::Type::div:
            result = RUNTIME_CALL(genericDiv, lhs, rhs);
            return;
        case ast::BinExp::Type::eq:
            result = RUNTIME_CALL(genericEq, lhs, rhs);
            return;
        case ast::BinExp::Type::neq:
            result = RUNTIME_CALL(genericNeq, lhs, rhs);
            return;
        case ast::BinExp::Type::lt:
            result = RUNTIME_CALL(genericLt, lhs, rhs);
            return;
        case ast::BinExp::Type::gt:
            result = RUNTIME_CALL(genericGt, lhs, rhs);
            return;
        default: // can't happen
            return;
        }
    }

  /** Rift Function Call. First obtain the function pointer, then the
      arguments.
   */
  void visit(ast::UserCall * node) override {
    node->name->accept(this);
    std::vector<Value *> args;
    args.push_back(result);
    args.push_back(fromInt(node->args.size()));
    for (ast::Exp * arg : node->args) {
      arg->accept(this);
      args.push_back(result);
    }
    result = CallInst::Create(m->call, args, "", b);
  }

  /** Call the runtime function length, box the scalar result into a vector
      and then an RVal.
  */
  void visit(ast::LengthCall * node) override {
    node->args[0]->accept(this);
    result = RUNTIME_CALL(length, result);
    result = RUNTIME_CALL(doubleVectorLiteral, result);
    result = RUNTIME_CALL(fromDoubleVector, result);
  }

    /** Call to type() function is a call to type() runtime and then boxing of the character vector. 
     */
    void visit(ast::TypeCall * node)  override {
        node->args[0]->accept(this);
        result = RUNTIME_CALL(type, result);
        result = RUNTIME_CALL(fromCharacterVector, result);
    }

    /** Eval call is translated to runtime eval function. 
     */
    void visit(ast::EvalCall * node)  override {
        node->args[0]->accept(this);
        result = RUNTIME_CALL(genericEval, env, result);
    }

    /** C call is translated to the concatenation runtime call. 
     */
    void visit(ast::CCall * node)  override {
        std::vector<llvm::Value *> args;
        args.push_back(fromInt(static_cast<int>(node->args.size())));
        for (ast::Exp * arg : node->args) {
            arg->accept(this);
            args.push_back(result);
        }
        result = CallInst::Create(m->c, args, "", b);
    }

    /** Index read conpiles the source, and indices expressions and then calls the getElement runtime. 
     */
    void visit(ast::Index * node) override {
        node->name->accept(this);
        llvm::Value * obj = result;
        node->index->accept(this);
        result = RUNTIME_CALL(genericGetElement, obj, result);
    }

  /** Assign a variable.
   */
  void visit(ast::SimpleAssignment * node) override {
    node->rhs->accept(this);
    RUNTIME_CALL(envSet, env, fromInt(node->name->symbol), result);
  }

  /** Assign into a vector at an index.
   */
  void visit(ast::IndexAssignment * node) override {
    node->rhs->accept(this);
    llvm::Value * rhs = result;
    node->index->name->accept(this);
    llvm::Value * var = result;
    node->index->index->accept(this);
    RUNTIME_CALL(genericSetElement, var, result, rhs);
    result = rhs;
  }

  /** Conditionals in rift.  Compile the guard, convert the result to a boolean,
      and branch on that. PHI nodes have to be inserted when control flow merges
      after the conditional.
  */
  void visit(ast::IfElse * node) override {
    node->guard->accept(this);
    llvm::Value * guard = RUNTIME_CALL(toBoolean, result);
    // Basic blocks and guard...
    BasicBlock * ifTrue = BasicBlock::Create(getGlobalContext(), "trueCase", f, nullptr);
    BasicBlock * ifFalse = BasicBlock::Create(getGlobalContext(), "falseCase", f, nullptr);
    BasicBlock * merge = BasicBlock::Create(getGlobalContext(), "afterIf", f, nullptr);
    // Branch...
    BranchInst::Create(ifTrue, ifFalse, guard, b);
    // Current BB set to true case, compile, remember result and merge
    b = ifTrue;
    node->ifClause->accept(this);
    llvm::Value * trueResult = result;
    BranchInst::Create(merge, b);
    // remember the last basic block of the case (this will denote the incomming path to the phi node)
    ifTrue = b;
    // do the same for else case
    b = ifFalse;
    node->elseClause->accept(this);
    llvm::Value * falseResult = result;
    BranchInst::Create(merge, b);
    ifFalse = b;
    // Set BB to merge point
    b = merge;
    // Emit PHI node with values coming from the if and else cases
    PHINode * phi = PHINode::Create(type::ptrValue, 2, "ifPhi", b);
    phi->addIncoming(trueResult, ifTrue);
    phi->addIncoming(falseResult, ifFalse);
    result = phi;
  }

    /** Compiles while loop.

    Due to the simple nature of the while loop, we do not have to worry about phi nodes, which would normally pop up in loop as well as in conditionals. 
    */
    void visit(ast::WhileLoop * node) override {
        // create basic blocks for the loop start (evaluation of the guard), loop body, and the block after the loop, when we exit it
        BasicBlock * loopStart = BasicBlock::Create(getGlobalContext(), "loopStart", f, nullptr);
        BasicBlock * loopBody = BasicBlock::Create(getGlobalContext(), "loopBody", f, nullptr);
        BasicBlock * loopNext = BasicBlock::Create(getGlobalContext(), "afterLoop", f, nullptr);
        // jump to loop start 
        BranchInst::Create(loopStart, b);
        // compile loop start as the evaluation of the guard and conditional branch to the body, or after the loop
        b = loopStart;
        node->guard->accept(this);
        llvm::Value * whileResult = result;
        llvm::Value * guard = RUNTIME_CALL(toBoolean, result);
        BranchInst::Create(loopBody, loopNext, guard, b);
        // compile the loop body, at the end of the loop body, branch to loop start to evaluate the guard again
        b = loopBody;
        node->body->accept(this);
        BranchInst::Create(loopStart, b);
        // set the current basic block to the one after the loop, the result is the guard (just because the result must be soething)
        b = loopNext;
        result = whileResult;
    }

private:
    /** Current BB */
    BasicBlock * b;

    /** Current function. */
    llvm::Function * f;
    
    /** Current Module */
    RiftModule * m;

    /** Result of visit functions */
    llvm::Value * result;

    /* Current Environment */
    llvm::Value * env;
};


FunPtr compile(ast::Fun * what) {
  Compiler c;
  return c.compile(what);
}

} // namespace rift



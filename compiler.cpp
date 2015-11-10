#include "parser.h"
#include "runtime.h"
#include "compiler.h"
#include "pool.h"

#include <initializer_list>
using namespace llvm;

namespace rift {

namespace type {

/** Initialization type declarations. Each Rift type must be declared to
    LLVM.
  */
#define STRUCT(name, ...) \
    StructType::create(name, __VA_ARGS__, nullptr)
#define FUN_TYPE(result, ...) \
    FunctionType::get(result, std::vector<llvm::Type*>({ __VA_ARGS__}), false)
#define FUN_TYPE_VARARG(result, ...) \
    FunctionType::get(result, std::vector<llvm::Type*>({ __VA_ARGS__}), true)

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

/** The Rift Memory manager extends the default LLVM memory manager with
    support for resolving the Rift runtime functions. This is achieved by
    extending the behavior of the getSymbolAddress function.
  */
class MemoryManager : public llvm::SectionMemoryManager {

public:
#define NAME_IS(name) if (Name == #name) return reinterpret_cast<uint64_t>(::name)
    /** Return the address of symbol, or nullptr if undefind. We extend the
        default LLVM resolution with the list of RIFT runtime functions.
      */
    uint64_t getSymbolAddress(const std::string & Name) override {
        uint64_t addr = SectionMemoryManager::getSymbolAddress(Name);
        if (addr != 0) return addr;
        // This bit is for some OSes (Windows and OSX where the MCJIT symbol
        // loading is broken)
        NAME_IS(envCreate);
        NAME_IS(envGet);
        NAME_IS(envSet);
        NAME_IS(doubleVectorLiteral);
        NAME_IS(characterVectorLiteral);
        NAME_IS(fromDoubleVector);
        NAME_IS(fromCharacterVector);
        NAME_IS(fromFunction);
        NAME_IS(genericGetElement);
        NAME_IS(genericSetElement);
        NAME_IS(genericAdd);
        NAME_IS(genericSub);
        NAME_IS(genericMul);
        NAME_IS(genericDiv);
        NAME_IS(genericEq);
        NAME_IS(genericNeq);
        NAME_IS(genericLt);
        NAME_IS(genericGt);
        NAME_IS(createFunction);
        NAME_IS(toBoolean);
        NAME_IS(call);
        NAME_IS(length);
        NAME_IS(type);
        NAME_IS(eval);
        NAME_IS(genericEval);
        NAME_IS(c);
        report_fatal_error("Extern function '" + Name + "' couldn't be resolved!");
    }
};

/** 
    The compiler: a visitor over the AST.
*/
class Compiler : public Visitor {
public:
    /** Creates the module to which the function will be compiled.
    */
    Compiler() : m(new RiftModule()) {}

    /** Compiles a function and returns a pointer to the native code.  JIT
      compilation in LLVM finalizes the module, this function can only be
      called once.
      */
    FunPtr compile(ast::Fun * what) {
        unsigned start = Pool::functionsCount();
        int result = compileFunction(what);
        ExecutionEngine * engine =
            EngineBuilder(std::unique_ptr<Module>(m))
                .setMCJITMemoryManager(
                        std::unique_ptr<MemoryManager>(new MemoryManager()))
                .create();
        engine->finalizeObject();
        // Compile newly registered functions; update their native code in the
        // registered functions vector
        for (; start < Pool::functionsCount(); ++start) {
            RFun * rec = Pool::getFunction(start);
            rec->code = reinterpret_cast<FunPtr>(engine->getPointerToFunction(rec->bitcode));
        }
        return Pool::getFunction(result)->code;
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

    /** Runtime function call. The first argument is the name of a runtime
      function defined in the RiftModule. The remaining arguments are passed
      to the function. The string is the name of the register where the result
      of the call will be stored, when empty, LLVM picks. The last argument is
      the BB where to append. */
#define RUNTIME_CALL(name, ...) \
    CallInst::Create(m->name,				     \
            std::vector<llvm::Value*>({__VA_ARGS__}), \
            "", \
            b)

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

    /** Function declaration.  Compiles function, use its id as a constant
        for createFunction() which binding the function code to the
        environment. Box result into a value.
      */
    void visit(ast::Fun * node) override {
        int fi = compileFunction(node);
        result = RUNTIME_CALL(createFunction, fromInt(fi), env);
        result = RUNTIME_CALL(fromFunction, result);
    }

    /** Binary expression. First compile arguments and then call respective
        runtime function.
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

    /** Rift Function Call. First obtain the function pointer, then arguments. */
    void visit(ast::UserCall * node) override {
        throw "Caaally caaaly caaaly call";
    }

    /** Call length runtime, box the scalar result  */
    void visit(ast::LengthCall * node) override {
        node->args[0]->accept(this);
        result = RUNTIME_CALL(length, result);
        result = RUNTIME_CALL(doubleVectorLiteral, result);
        result = RUNTIME_CALL(fromDoubleVector, result);
    }

    /** Call type runtime and then boxing of the character vector. */
    void visit(ast::TypeCall * node)  override {
        node->args[0]->accept(this);
        result = RUNTIME_CALL(type, result);
        result = RUNTIME_CALL(fromCharacterVector, result);
    }

    /** Eval.  */
    void visit(ast::EvalCall * node)  override {
        node->args[0]->accept(this);
        result = RUNTIME_CALL(genericEval, env, result);
    }

    /** Concatenate.  */
    void visit(ast::CCall * node)  override {
        std::vector<llvm::Value *> args;
        args.push_back(fromInt(static_cast<int>(node->args.size())));
        for (ast::Exp * arg : node->args) {
            arg->accept(this);
            args.push_back(result);
        }
        result = CallInst::Create(m->c, args, "", b);
    }

    /** Indexed read.  */
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

    /** Conditional.  Compile the guard, convert the result to a boolean,
       and branch on that. PHI nodes have to be inserted when control flow
       merges after the conditional.
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
        // remember the last BB of the case (this will denote the incomming path to the phi node)
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

    /** While. The loop is simple enough that we don't have to worry about
	PHI nodes.
      */
    void visit(ast::WhileLoop * node) override {
        throw "Looopy looopy looopy loop";
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



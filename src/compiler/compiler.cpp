#include "rift.h"
#include "compiler.h"
#include "pool.h"

using namespace llvm;

/** Shorthand for calling runtime functions.  */
#define RUNTIME_CALL(NAME, ...) cur.b->CreateCall(                 \
        NAME(m.get()), vector<Value*>({ __VA_ARGS__ }), "")

namespace {

    /* Declares function with given name and signature. 
     */
    Function * declareFunction(char const * name, FunctionType * signature, Module * m) {
        return Function::Create(signature, Function::ExternalLinkage, name, m);
    }

    /* Declares functon with given name and signature and marks it as pure (ReadNone in LLVM's jargon). 
     */
    Function * declarePureFunction(char const * name, FunctionType * signature, Module * m) {
        Function * f = declareFunction(name, signature, m);
        AttributeSet as;
        AttrBuilder b;
        b.addAttribute(Attribute::ReadNone);
        as = AttributeSet::get(rift::Compiler::context(),AttributeSet::FunctionIndex, b);
        f->setAttributes(as);
        return f;
    }
}

namespace rift {

Compiler::Compiler():
    result(nullptr),
    m(new Module("module", context())) {
}

#if VERSION == 0
int Compiler::compile(ast::Fun * n) {
    // Create the function and its first BB
    auto f = Function::Create(
            type::NativeCode, Function::ExternalLinkage, "riftFunction", m.get());

    BasicBlock * entry = BasicBlock::Create(context(), "entry", f, nullptr);
    auto b = new IRBuilder<>(entry);

    // Get the (single) argument of the function and store is as the
    // environment
    Function::arg_iterator args = f->arg_begin();
    auto env = &*args;
    env->setName("env");

    Function * doubleVectorLiteral =
        declarePureFunction("doubleVectorLiteral", type::v_d, m.get());

    result = b->CreateCall(doubleVectorLiteral,
            vector<Value*>( {ConstantFP::get(context(), APFloat(0.0f))}), "");

    // Append return instruction of the last used value
    b->CreateRet(result);

    // Register and get index
    int idx = Pool::addFunction(n, f);
    f->setName(STR(idx));
    return idx;
}
#endif //VERSION

#if VERSION > 0
/* Createas a function that checks if given function is present in the module, and if not appends it, returning the declared function. 
 */
#define FUN_PURE(NAME, SIGNATURE) Function * Compiler::NAME(Module * m) { \
    Function * result = m->getFunction(#NAME); \
    if (result == nullptr) \
        result = declarePureFunction(#NAME, SIGNATURE, m); \
    return result; \
}

/* Createas a function that checks if given function is present in the module, and if not appends it, returning the declared function. 
 */
#define FUN(NAME, SIGNATURE) Function * Compiler::NAME(Module * m) { \
    Function * result = m->getFunction(#NAME); \
    if (result == nullptr) \
        result = declareFunction(#NAME, SIGNATURE, m); \
    return result; \
}

/* We now call the RUNTIME_FUNCTIONS macro which will expand the two macros above and create proper functions for all runtime functions we have. 
 */
RUNTIME_FUNCTIONS
#undef FUN_PURE
#undef FUN


Compiler::FunctionContext::FunctionContext(string name, Module* m) {
    // Create the function
    f = Function::Create(type::NativeCode, Function::ExternalLinkage,
                         name, m);
    // Create the first BB and a builder
    BasicBlock * entry =
        BasicBlock::Create(context(), "entry", f, nullptr);
    b = new IRBuilder<>(entry);
    // Rift ABI specifies the initial environment as only argument.
    // All function args are bound in there.
    Function::arg_iterator args = f->arg_begin();
    env = &*args;
    env->setName("env");
}

int Compiler::compile(ast::Fun * n) {
    // Backup context in case we are creating a nested function
    FunctionContext oldContext(cur);
    // Create a new function context
    cur = FunctionContext("riftFunction", m.get());
    // if the function is empty, return 0 as default return value
    if (n->body->body.empty()) {
        result = RUNTIME_CALL(doubleVectorLiteral, fromDouble(0));
    // otherwise compile the function
    } else {
        result = nullptr;
        n->body->accept(this);
        assert(result);
    }
    // Append return instruction of the last used value
    cur.b->CreateRet(result);
    // Register and get index
    int idx = Pool::addFunction(n, cur.f);
    cur.f->setName(STR(idx));
    // Restore context
    cur.restore(oldContext);
    return idx;
}

/** Safeguard against forgotten  visitor methods.   */
void Compiler::visit(ast::Exp * n) {
    throw "Unexpected: You are missing a visit() method.";
}

/** Get  double, box it into a vector of length 1, box that into a Rift Value. */
void Compiler::visit(ast::Num * n) {
    result = RUNTIME_CALL(doubleVectorLiteral, fromDouble(n->value));
}

/** Similarly string is loaded as character vector and then boxed into value. */
void Compiler::visit(ast::Str * n) {
    result = RUNTIME_CALL(characterVectorLiteral, fromInt(n->index));
}

/** Read the variable from environment. */
void Compiler::visit(ast::Var * n) {
    result = RUNTIME_CALL(envGet, cur.env, fromInt(n->symbol));
}

/** Compile each statement, the last one is the result. */
void Compiler::visit(ast::Seq * n) {
    for (ast::Exp * e : n->body)  e->accept(this);
}

/** Function declaration.  Compiles function, use its id as a constant
    for createFunction() which binding the function code to the
    environment. Box result into a value. */
void Compiler::visit(ast::Fun * n) {
    int fi = compile(n);
    result = RUNTIME_CALL(createFunction, fromInt(fi), cur.env);
}

/** Binary expression. First compile arguments and then call respective
    runtime function.  */
void Compiler::visit(ast::BinExp * n) {
    n->lhs->accept(this);
    Value * lhs = result;
    n->rhs->accept(this);
    Value * rhs = result;
    switch (n->op) {
        case ast::BinExp::Op::add:
            result = RUNTIME_CALL(genericAdd, lhs, rhs);
            return;
        case ast::BinExp::Op::sub:
            result = RUNTIME_CALL(genericSub, lhs, rhs);
            return;
        case ast::BinExp::Op::mul:
            result = RUNTIME_CALL(genericMul, lhs, rhs);
            return;
        case ast::BinExp::Op::div:
            result = RUNTIME_CALL(genericDiv, lhs, rhs);
            return;
        case ast::BinExp::Op::eq:
            result = RUNTIME_CALL(genericEq, lhs, rhs);
            return;
        case ast::BinExp::Op::neq:
            result = RUNTIME_CALL(genericNeq, lhs, rhs);
            return;
        case ast::BinExp::Op::lt:
            result = RUNTIME_CALL(genericLt, lhs, rhs);
            return;
        case ast::BinExp::Op::gt:
            result = RUNTIME_CALL(genericGt, lhs, rhs);
            return;
        default: // can't happen
            return;
    }
}

/** Rift Function Call. First obtain the function pointer, then arguments. */
void Compiler::visit(ast::UserCall * n) {
#if VERSION >= 5
    n->name->accept(this);
    vector<Value *> args;
    args.push_back(result);
    args.push_back(fromInt(n->args.size()));
    for (ast::Exp * arg : n->args) {
        arg->accept(this);
        args.push_back(result);
    }
    result = cur.b->CreateCall(call(m.get()), args, "");
#endif //VERSION
#if VERSION < 5
    //TODO
    assert(false);
#endif //VERSION
}

/** Call length runtime, box the scalar result  */
void Compiler::visit(ast::LengthCall * n) {
    n->args[0]->accept(this);
    result = RUNTIME_CALL(length, result);
    result = RUNTIME_CALL(doubleVectorLiteral, result);
}

/** Call type runtime and then boxing of the character vector. */
void Compiler::visit(ast::TypeCall * n) {
    n->args[0]->accept(this);
    result = RUNTIME_CALL(type, result);
}

/** Eval.  */
void Compiler::visit(ast::EvalCall * n) {
    n->args[0]->accept(this);
    result = RUNTIME_CALL(genericEval, cur.env, result);
}

/** Concatenate.  */
void Compiler::visit(ast::CCall * n) {
    vector<Value *> args;
    args.push_back(fromInt(static_cast<int>(n->args.size())));
    for (ast::Exp * arg : n->args) {
        arg->accept(this);
        args.push_back(result);
    }
    result = cur.b->CreateCall(c(m.get()), args, "");
}

/** Indexed read.  */
void Compiler::visit(ast::Index * n) {
    n->name->accept(this);
    Value * obj = result;
    n->index->accept(this);
    result = RUNTIME_CALL(genericGetElement, obj, result);
}

/** Assign a variable. */
void Compiler::visit(ast::SimpleAssignment * n) {
#if VERSION >= 5
    n->rhs->accept(this);
    RUNTIME_CALL(envSet, cur.env, fromInt(n->name->symbol), result);
#endif //VERSION
#if VERSION < 5
    assert(false);
#endif //VERSION
}

/** Assign into a vector at an index. */
void Compiler::visit(ast::IndexAssignment * n) {
    n->rhs->accept(this);
    Value * rhs = result;
    n->index->name->accept(this);
    Value * var = result;
    n->index->index->accept(this);
    RUNTIME_CALL(genericSetElement, var, result, rhs);
    result = rhs;
}

/** Conditional.  Compile the guard, convert the result to a boolean,
   and branch on that. PHI ns have to be inserted when control flow
   merges after the conditional. */
void Compiler::visit(ast::IfElse * n) {
    // Create the basic blocks we will need
    BasicBlock * ifTrue = BasicBlock::Create(
            context(), "trueCase", cur.f, nullptr);
    BasicBlock * ifFalse = BasicBlock::Create(
            context(), "falseCase", cur.f, nullptr);
    BasicBlock * merge = BasicBlock::Create(
            context(), "afterIf", cur.f, nullptr);
    Value * trueResult;
    Value * falseResult;
#if VERSION < 5
    //TODO
    assert(false);
#endif //VERSION
#if VERSION >= 5
    // compile the condition
    n->guard->accept(this);
    Value * guard = RUNTIME_CALL(toBoolean, result);
    // do the conditional jump
    cur.b->CreateCondBr(guard, ifTrue, ifFalse);
    // flip the basic block to ifTrue, compile the true case and jump to
    // the merge block at the end
    cur.b->SetInsertPoint(ifTrue);
    n->ifClause->accept(this);
    trueResult = result;
    ifTrue = cur.b->GetInsertBlock();
    cur.b->CreateBr(merge);
    // flip the basic block to ifFalse, compile the else case and jump to
    // the merge block at the end
    cur.b->SetInsertPoint(ifFalse);
    n->elseClause->accept(this);
    falseResult = result;
    ifFalse = cur.b->GetInsertBlock();
    cur.b->CreateBr(merge);
#endif //VERSION
    // Set BB to merge point and emit a phi n for the then-else results
    cur.b->SetInsertPoint(merge);
    auto phi = cur.b->CreatePHI(type::ptrValue, 2, "ifPhi");
    phi->addIncoming(trueResult, ifTrue);
    phi->addIncoming(falseResult, ifFalse);
    // return the result of the phi n
    result = phi;
}

/** While. A loop always returns zero, so no need to worry about PHI nodes. */
void Compiler::visit(ast::WhileLoop * n) {
    // create BB for loop start (evaluation of the guard), loop body, and exit
    BasicBlock * guard = BasicBlock::Create(
            context(), "guard", cur.f, nullptr);
    BasicBlock * body = BasicBlock::Create(
            context(), "body", cur.f, nullptr);
    BasicBlock * cont = BasicBlock::Create(
            context(), "cont", cur.f, nullptr);
    // jump to start
    cur.b->CreateBr(guard);
    // compile start as the evaluation of the guard and conditional branch
    cur.b->SetInsertPoint(guard);
    // compile the guard condition
    n->guard->accept(this);
    Value * test = RUNTIME_CALL(toBoolean, result);
    cur.b->CreateCondBr(test, body, cont);
    // compile loop body, at the end of the loop body, branch to start
    cur.b->SetInsertPoint(body);
    n->body->accept(this);
    cur.b->CreateBr(guard);
    // set the current BB to the one after the loop, the result is the
    // value of the last instruction
    cur.b->SetInsertPoint(cont);
    // we need a default value for the loop to evaluate to
    result = RUNTIME_CALL(doubleVectorLiteral, fromDouble(0));
}
#endif //VERSION
}

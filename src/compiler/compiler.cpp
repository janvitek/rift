#include "rift.h"
#include "compiler.h"
#include "pool.h"


/** Shorthand for calling runtime functions.  */
#define RUNTIME_CALL(NAME, ...) cur.b->CreateCall(                 \
        NAME(m.get()), vector<llvm::Value*>({ __VA_ARGS__ }), "")

namespace {

    llvm::Function * declareFunction(char const * name, llvm::FunctionType * signature, llvm::Module * m) {
        return llvm::Function::Create(signature, llvm::Function::ExternalLinkage, name, m);
    }


    llvm::Function * declarePureFunction(char const * name, llvm::FunctionType * signature, llvm::Module * m) {
        llvm::Function * f = declareFunction(name, signature, m);
        llvm::AttributeSet as;
        llvm::AttrBuilder b;
        b.addAttribute(llvm::Attribute::ReadNone);
        as = llvm::AttributeSet::get(rift::Compiler::context(),llvm::AttributeSet::FunctionIndex, b);
        f->setAttributes(as);
        return f;
    }

}

namespace rift {


Compiler::Compiler():
    result(nullptr),
    m(new llvm::Module("module", context())) {
}

#if VERSION == 0
int Compiler::compile(ast::Fun * node) {
    // Create the function and its first BB
    cur.f = llvm::Function::Create(type::NativeCode, llvm::Function::ExternalLinkage, "riftFunction", m.get());

    llvm::BasicBlock * entry = llvm::BasicBlock::Create(context(), "entry", f, nullptr);
    cur.b = new llvm::IRBuilder<>(entry);

    // Get the (single) argument of the function and store is as the
    // environment
    llvm::Function::arg_iterator args = f->arg_begin();
    cur.env = &*args;
    env->setName("env");

    llvm::Function * doubleVectorLiteral =
        declarePureFunction("doubleVectorLiteral", type::v_d, m.get());

    // TODO: return something else than returning 0
    result = b->CreateCall(doubleVectorLiteral,
            vector<llvm::Value*>(
                {llvm::ConstantFP::get(context(), llvm::APFloat(0.0f))}), "");

    // Append return instruction of the last used value
    b->CreateRet(result);

    // Register and get index
    int idx = Pool::addFunction(node, f);
    f->setName(STR(idx));
    return idx;
}
#endif //VERSION

#if VERSION > 0
#define FUN_PURE(NAME, SIGNATURE) llvm::Function * Compiler::NAME(llvm::Module * m) { \
    llvm::Function * result = m->getFunction(#NAME); \
    if (result == nullptr) \
        result = declarePureFunction(#NAME, SIGNATURE, m); \
    return result; \
}

#define FUN(NAME, SIGNATURE) llvm::Function * Compiler::NAME(llvm::Module * m) { \
    llvm::Function * result = m->getFunction(#NAME); \
    if (result == nullptr) \
        result = declareFunction(#NAME, SIGNATURE, m); \
    return result; \
}
RUNTIME_FUNCTIONS
#undef FUN_PURE
#undef FUN

Compiler::FunctionContext::FunctionContext(string name, llvm::Module* m) {
    // Create the function
    f = llvm::Function::Create(
            type::NativeCode, llvm::Function::ExternalLinkage,
            name, m);
    // Create the first BB and a builder
    llvm::BasicBlock * entry = llvm::BasicBlock::Create(
            context(), "entry", f, nullptr);
    b = new llvm::IRBuilder<>(entry);
    // Rift ABI specifies the initial environment as only argument.
    // All function args are bound in there.
    llvm::Function::arg_iterator args = f->arg_begin();
    env = &*args;
    env->setName("env");
}

int Compiler::compile(ast::Fun * node) {
    // Backup context in case we are creating a nested function
    FunctionContext oldContext(cur);

    // Create a new function context
    cur = FunctionContext("riftFunction", m.get());

    // if the function is empty, return 0 as default return value
    if (node->body->body.empty()) {
        result = RUNTIME_CALL(doubleVectorLiteral, fromDouble(0));
    // otherwise compile the function
    } else {
        result = nullptr;
        node->body->accept(this);
        assert(result);
    }

    // Append return instruction of the last used value
    cur.b->CreateRet(result);

    // Register and get index
    int idx = Pool::addFunction(node, cur.f);
    cur.f->setName(STR(idx));

    // Restore context
    cur.restore(oldContext);
    return idx;
}

/** Safeguard against forgotten  visitor methods.   */
void Compiler::visit(ast::Exp * node) {
    throw "Unexpected: You are missing a visit() method.";
}

/** Get the double value, box it into a vector of length 1, box that into
  a Rift Value.
  */
void Compiler::visit(ast::Num * node) {
    result = RUNTIME_CALL(doubleVectorLiteral, fromDouble(node->value));
}

/** Similarly string is loaded as character vector and then boxed into value.
*/
void Compiler::visit(ast::Str * node) {
    result = RUNTIME_CALL(characterVectorLiteral, fromInt(node->index));
}

/** Variable translates into reading from environment.
*/
void Compiler::visit(ast::Var * node) {
    result = RUNTIME_CALL(envGet, cur.env, fromInt(node->symbol));
}

/** Sequence is compilation of each of its elements. The last one will stay
 * in the result.
*/
void Compiler::visit(ast::Seq * node) {
    for (ast::Exp * e : node->body)
        e->accept(this);
}

/** Function declaration.  Compiles function, use its id as a constant
    for createFunction() which binding the function code to the
    environment. Box result into a value.
  */
void Compiler::visit(ast::Fun * node) {
    int fi = compile(node);
    result = RUNTIME_CALL(createFunction, fromInt(fi), cur.env);
}

/** Binary expression. First compile arguments and then call respective
    runtime function.
  */
void Compiler::visit(ast::BinExp * node) {
    node->lhs->accept(this);
    llvm::Value * lhs = result;
    node->rhs->accept(this);
    llvm::Value * rhs = result;
    switch (node->op) {
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
void Compiler::visit(ast::UserCall * node) {
#if VERSION >= 5
    node->name->accept(this);

    vector<llvm::Value *> args;
    args.push_back(result);
    args.push_back(fromInt(node->args.size()));

    for (ast::Exp * arg : node->args) {
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
void Compiler::visit(ast::LengthCall * node) {
    node->args[0]->accept(this);
    result = RUNTIME_CALL(length, result);
    result = RUNTIME_CALL(doubleVectorLiteral, result);
}

/** Call type runtime and then boxing of the character vector. */
void Compiler::visit(ast::TypeCall * node) {
    node->args[0]->accept(this);
    result = RUNTIME_CALL(type, result);
}

/** Eval.  */
void Compiler::visit(ast::EvalCall * node) {
    node->args[0]->accept(this);
    result = RUNTIME_CALL(genericEval, cur.env, result);
}

/** Concatenate.  */
void Compiler::visit(ast::CCall * node) {
    vector<llvm::Value *> args;
    args.push_back(fromInt(static_cast<int>(node->args.size())));
    for (ast::Exp * arg : node->args) {
        arg->accept(this);
        args.push_back(result);
    }
    result = cur.b->CreateCall(c(m.get()), args, "");
}

/** Indexed read.  */
void Compiler::visit(ast::Index * node) {
    node->name->accept(this);
    llvm::Value * obj = result;
    node->index->accept(this);
    result = RUNTIME_CALL(genericGetElement, obj, result);
}

/** Assign a variable.
*/
void Compiler::visit(ast::SimpleAssignment * node) {
#if VERSION >= 5
    node->rhs->accept(this);
    RUNTIME_CALL(envSet, cur.env, fromInt(node->name->symbol), result);
#endif //VERSION
#if VERSION < 5
    assert(false);
#endif //VERSION
}

/** Assign into a vector at an index.
*/
void Compiler::visit(ast::IndexAssignment * node) {
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
void Compiler::visit(ast::IfElse * node) {
#if VERSION >= 5
    // Create the basic blocks we will need
    llvm::BasicBlock * ifTrue = llvm::BasicBlock::Create(
            context(), "trueCase", cur.f, nullptr);
    llvm::BasicBlock * ifFalse = llvm::BasicBlock::Create(
            context(), "falseCase", cur.f, nullptr);
    llvm::BasicBlock * merge = llvm::BasicBlock::Create(
            context(), "afterIf", cur.f, nullptr);

    // compile the condition
    node->guard->accept(this);
    llvm::Value * guard = RUNTIME_CALL(toBoolean, result);

    // do the conditional jump
    cur.b->CreateCondBr(guard, ifTrue, ifFalse);

    // flip the basic block to ifTrue, compile the true case and jump to
    // the merge block at the end
    cur.b->SetInsertPoint(ifTrue);
    node->ifClause->accept(this);
    llvm::Value * trueResult = result;
    ifTrue = cur.b->GetInsertBlock();
    cur.b->CreateBr(merge);

    // flip the basic block to ifFalse, compile the else case and jump to
    // the merge block at the end
    cur.b->SetInsertPoint(ifFalse);
    node->elseClause->accept(this);
    llvm::Value * falseResult = result;
    ifFalse = cur.b->GetInsertBlock();
    cur.b->CreateBr(merge);

    // Set BB to merge point and emit a phi node for the then-else results
    cur.b->SetInsertPoint(merge);
    llvm::PHINode * phi = cur.b->CreatePHI(type::ptrValue, 2, "ifPhi");
    phi->addIncoming(trueResult, ifTrue);
    phi->addIncoming(falseResult, ifFalse);

    // return the result of the phi node
    result = phi;
#endif //VERSION
#if VERSION < 5
    //TODO
    assert(false);
#endif //VERSION
}

/** While. The loop is simple enough that we don't have to worry about
PHI nodes.
  */
void Compiler::visit(ast::WhileLoop * node) {
    // create BB for loop start (evaluation of the guard), loop body, and exit
    llvm::BasicBlock * guard = llvm::BasicBlock::Create(
            context(), "guard", cur.f, nullptr);
    llvm::BasicBlock * body = llvm::BasicBlock::Create(
            context(), "body", cur.f, nullptr);
    llvm::BasicBlock * cont = llvm::BasicBlock::Create(
            context(), "cont", cur.f, nullptr);

    // jump to start
    cur.b->CreateBr(guard);

    // compile start as the evaluation of the guard and conditional branch
    cur.b->SetInsertPoint(guard);

    // compile the guard condition
    node->guard->accept(this);
    llvm::Value * test = RUNTIME_CALL(toBoolean, result);

    cur.b->CreateCondBr(test, body, cont);

    // compile loop body, at the end of the loop body, branch to start
    cur.b->SetInsertPoint(body);
    node->body->accept(this);
    cur.b->CreateBr(guard);

    // set the current BB to the one after the loop, the result is the
    // value of the last instruction
    cur.b->SetInsertPoint(cont);

    // we need a default value for the loop to evaluate to
    result = RUNTIME_CALL(doubleVectorLiteral, fromDouble(0));
}

#endif //VERSION

}

#include "parser.h"
#include "runtime.h"

#include <initializer_list>
using namespace llvm;

namespace rift {

namespace type {
#define STRUCT(name, ...) StructType::create(name, __VA_ARGS__, nullptr)
#define FUN_TYPE(result, ...) FunctionType::get(result, std::vector<Type*>({ __VA_ARGS__}), false)
#define FUN_TYPE_VARARG(result, ...) FunctionType::get(result, std::vector<Type*>({ __VA_ARGS__}), true)

    StructType * environmentType();

    Type * Void = Type::getVoidTy(getGlobalContext());
    Type * Int = IntegerType::get(getGlobalContext(), 32);
    Type * Double = Type::getDoubleTy(getGlobalContext());
    Type * Character = IntegerType::get(getGlobalContext(), 8);
    Type * Bool = IntegerType::get(getGlobalContext(), 1);

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
    FunctionType * v_eiv = FUN_TYPE(ptrValue, ptrEnvironment, Int, ptrValue);

    FunctionType * v_f = FUN_TYPE(ptrValue, ptrFunction);
    FunctionType * f_ie = FUN_TYPE(ptrFunction, Int, ptrEnvironment);

    FunctionType * b_v = FUN_TYPE(Bool, ptrValue);

    FunctionType * v_veiVA = FUN_TYPE_VARARG(ptrValue, ptrValue, ptrEnvironment, Int);

    FunctionType * void_vvv = FUN_TYPE(Void, ptrValue, ptrValue, ptrValue);

    FunctionType * d_v = FUN_TYPE(Double, ptrValue);
    FunctionType * cv_v = FUN_TYPE(ptrCharacterVector, ptrValue);
    FunctionType * v_iVA = FUN_TYPE_VARARG(ptrValue, Int);

    StructType * environmentType() {
        StructType * result = StructType::create(getGlobalContext(), "Environment");
        ptrEnvironment = PointerType::get(result, 0);
        result->setBody(ptrEnvironment, ptrBinding, Int, nullptr);
        return result;
    }
}

class MemoryManager : public llvm::SectionMemoryManager {
    MemoryManager(const MemoryManager&) = delete;
    void operator=(const MemoryManager&) = delete;

public:
    MemoryManager() = default;
    virtual ~MemoryManager() {}

#define CHECK_RUNTIME(name) if (Name == #name) return reinterpret_cast<uint64_t>(::name)
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
            CHECK_RUNTIME(doubleGetElement);
            CHECK_RUNTIME(characterGetElement);
            CHECK_RUNTIME(genericGetElement);
            CHECK_RUNTIME(doubleSetElement);
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
            CHECK_RUNTIME(functionEq);
            CHECK_RUNTIME(genericEq);
            CHECK_RUNTIME(doubleNeq);
            CHECK_RUNTIME(characterNeq);
            CHECK_RUNTIME(functionNeq);
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
            CHECK_RUNTIME(c);
            report_fatal_error("Program used extern function '" + Name +
                "' which could not be resolved!");
        }
        return addr;

    }

};

class RiftModule : public Module {
public:
    RiftModule():
        Module("rift", getGlobalContext()) {
    }

#define DEF_FUN(name, signature) llvm::Function * name = llvm::Function::Create(signature, llvm::Function::ExternalLinkage, #name, this)
    DEF_FUN(doubleVectorLiteral, type::dv_d);
    DEF_FUN(characterVectorLiteral, type::cv_i);
    DEF_FUN(fromDoubleVector, type::v_dv);
    DEF_FUN(fromCharacterVector, type::v_cv);
    DEF_FUN(fromFunction, type::v_f);
    DEF_FUN(genericGetElement, type::v_vv);
    DEF_FUN(genericSetElement, type::void_vvv);
    DEF_FUN(envGet, type::v_ei);
    DEF_FUN(envSet, type::v_eiv);
    DEF_FUN(genericAdd, type::v_vv);
    DEF_FUN(genericSub, type::v_vv);
    DEF_FUN(genericMul, type::v_vv);
    DEF_FUN(genericDiv, type::v_vv);
    DEF_FUN(genericEq, type::v_vv);
    DEF_FUN(genericNeq, type::v_vv);
    DEF_FUN(genericLt, type::v_vv);
    DEF_FUN(genericGt, type::v_vv);
    DEF_FUN(createFunction, type::f_ie);
    DEF_FUN(toBoolean, type::b_v);
    DEF_FUN(call, type::v_veiVA);
    DEF_FUN(length, type::d_v);
    DEF_FUN(type, type::cv_v);
    DEF_FUN(genericEval, type::v_ev);
    DEF_FUN(c, type::v_iVA);

};


class Compiler : public Visitor {
public:
    Compiler():
        m(new RiftModule()) {
    }

    FunPtr compile(ast::Fun * what) {
        unsigned start = Runtime::functionsCount();
        int result = compileFunction(what);
        // now do the actual JITting
        ExecutionEngine * engine = EngineBuilder(std::unique_ptr<Module>(m)).setMCJITMemoryManager(std::unique_ptr<MemoryManager>(new MemoryManager())).create();
        engine->finalizeObject();
        for (; start < Runtime::functionsCount(); ++start) {
            ::Function * rec = Runtime::getFunction(start);
            rec->code = reinterpret_cast<FunPtr>(engine->getPointerToFunction(rec->bitcode));
        }
        return Runtime::getFunction(result)->code;
    }

    int compileFunction(ast::Fun * node) {
        llvm::Function * oldF = f;
        BasicBlock * oldB = b;
        llvm::Value * oldEnv = env;
        f = llvm::Function::Create(type::NativeCode, llvm::Function::ExternalLinkage, "riftFunction", m);
        b = BasicBlock::Create(getGlobalContext(), "entry", f, nullptr);
        llvm::Function::arg_iterator args = f->arg_begin();
        env = args++;
        env->setName("env");
        node->body->accept(this);
        ReturnInst::Create(getGlobalContext(), result, b);
        //f->dump();
        int result = Runtime::addFunction(node, f);
        f = oldF;
        b = oldB;
        env = oldEnv;
        return result;
    }


#define RUNTIME_CALL(name, ...) CallInst::Create(m->name, std::vector<llvm::Value*>({__VA_ARGS__}), "", b)


    llvm::Value * fromDouble(double value) {
        return ConstantFP::get(getGlobalContext(), APFloat(value));
    }

    llvm::Value * fromInt(int value) {
        return ConstantInt::get(getGlobalContext(), APInt(32, value));
    }

    void visit(ast::Exp * node) override {
        throw "Unexpected ast type reached during compilation";
    }

    void visit(ast::Num * node) override {
        result = RUNTIME_CALL(doubleVectorLiteral, fromDouble(node->value));
        result = RUNTIME_CALL(fromDoubleVector, result);
    }

    void visit(ast::Str * node) override {
        result = RUNTIME_CALL(characterVectorLiteral, fromInt(node->index));
        result = RUNTIME_CALL(fromCharacterVector, result);
    }

    void visit(ast::Var * node) override {
        result = RUNTIME_CALL(envGet, env, fromInt(node->symbol));
    }

    void visit(ast::Seq * node) override {
        for (ast::Exp * e : node->body)
            e->accept(this);
    }

    void visit(ast::Fun * node) override {
        int fi = compileFunction(node);
        result = RUNTIME_CALL(createFunction, fromInt(fi), env);
        result = RUNTIME_CALL(fromFunction, result);
    }

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
        default:
            // pass - can't happen
            return;
        }
    }


    void visit(ast::UserCall * node) override {
        // get the function we are going to call
        node->name->accept(this);
        std::vector<llvm::Value *> args;
        args.push_back(result);
        args.push_back(env);
        args.push_back(fromInt(node->args.size()));
        for (ast::Exp * arg : node->args) {
            arg->accept(this);
            args.push_back(result);
        }
        result = CallInst::Create(m->call, args, "", b);
    }

    void visit(ast::LengthCall * node) {
        node->args[0]->accept(this);
        result = RUNTIME_CALL(length, result);
        result = RUNTIME_CALL(doubleVectorLiteral, result);
        result = RUNTIME_CALL(fromDoubleVector, result);
    }

    void visit(ast::TypeCall * node) {
        node->args[0]->accept(this);
        result = RUNTIME_CALL(type, result);
        result = RUNTIME_CALL(fromCharacterVector, result);
    }

    void visit(ast::EvalCall * node) {
        node->args[0]->accept(this);
        result = RUNTIME_CALL(genericEval, env, result);
    }

    void visit(ast::CCall * node) {
        std::vector<llvm::Value *> args;
        args.push_back(fromInt(node->args.size()));
        for (ast::Exp * arg : node->args) {
            arg->accept(this);
            args.push_back(result);
        }
        result = CallInst::Create(m->c, args, "", b);
    }

    void visit(ast::Index * node) override {
        node->name->accept(this);
        llvm::Value * obj = result;
        node->index->accept(this);
        result = RUNTIME_CALL(genericGetElement, obj, result);
    }

    void visit(ast::SimpleAssignment * node) override {
        node->rhs->accept(this);
        result = RUNTIME_CALL(envSet, env, fromInt(node->name->symbol), result);
    }

    void visit(ast::IndexAssignment * node) override {
        node->rhs->accept(this);
        llvm::Value * rhs = result;
        node->index->name->accept(this);
        llvm::Value * var = result;
        node->index->index->accept(this);
        RUNTIME_CALL(genericSetElement, var, result, rhs);
        result = rhs;
    }

    void visit(ast::IfElse * node) override {
        node->guard->accept(this);
        llvm::Value * guard = RUNTIME_CALL(toBoolean, result);
        BasicBlock * ifTrue = BasicBlock::Create(getGlobalContext(), "trueCase", f, nullptr);
        BasicBlock * ifFalse = BasicBlock::Create(getGlobalContext(), "falseCase", f, nullptr);
        BasicBlock * next = BasicBlock::Create(getGlobalContext(), "afterIf", f, nullptr);
        BranchInst::Create(ifTrue, ifFalse, guard, b);
        b = ifTrue;
        node->ifClause->accept(this);
        llvm::Value * trueResult = result;
        BranchInst::Create(next, b);
        ifTrue = b;
        b = ifFalse;
        node->elseClause->accept(this);
        llvm::Value * falseResult = result;
        BranchInst::Create(next, b);
        ifFalse = b;
        b = next;
        PHINode * phi = PHINode::Create(type::ptrValue, 2, "ifPhi", b);
        phi->addIncoming(trueResult, ifTrue);
        phi->addIncoming(falseResult, ifFalse);
        result = phi;
    }

    void visit(ast::WhileLoop * node) override {
        BasicBlock * loopStart = BasicBlock::Create(getGlobalContext(), "loopStart", f, nullptr);
        BasicBlock * loopBody = BasicBlock::Create(getGlobalContext(), "loopBody", f, nullptr);
        BasicBlock * loopNext = BasicBlock::Create(getGlobalContext(), "afterLoop", f, nullptr);
        BranchInst::Create(loopStart, b);
        b = loopStart;
        node->guard->accept(this);
        llvm::Value * guard = RUNTIME_CALL(toBoolean, result);
        BranchInst::Create(loopBody, loopNext, guard, b);
        b = loopBody;
        node->body->accept(this);
        BranchInst::Create(loopStart, b);
        b = loopNext;
        result = guard;
    }

private:
    BasicBlock * b;
    llvm::Function * f;
    RiftModule * m;
    llvm::Value * result;
    llvm::Value * env;


};

FunPtr compile(ast::Fun * what) {
    Compiler c;
    return c.compile(what);
}

}



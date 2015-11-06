#include "parser.h"
#include "runtime.h"
#include "compiler.h"
#include "type_analysis.h"
#include "unboxing.h"
#include "boxing_removal.h"

#include <initializer_list>
using namespace llvm;

namespace rift {

namespace type {
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

    FunctionType * v_veiVA = FUN_TYPE_VARARG(ptrValue, ptrValue, ptrEnvironment, Int);

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
        // optimize functions in the module
        optimizeModule(engine);
        // finalize the module object so that we can compile it
        engine->finalizeObject();
        for (; start < Runtime::functionsCount(); ++start) {
            ::Function * rec = Runtime::getFunction(start);
            rec->code = reinterpret_cast<FunPtr>(engine->getPointerToFunction(rec->bitcode));
        }
        return Runtime::getFunction(result)->code;
    }

    void optimizeModule(ExecutionEngine * ee) {
        auto *pm = new legacy::FunctionPassManager(m);
        m->setDataLayout(* ee->getDataLayout());
        pm->add(new TypeAnalysis());
        pm->add(new Unboxing());
        pm->add(new BoxingRemoval());
        pm->add(createConstantPropagationPass());
        for (llvm::Function & f : *m) {
            pm->run(f);
            //f.dump();
        }
        delete pm;
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
        args.push_back(fromInt(static_cast<int>(node->args.size())));
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
        args.push_back(fromInt(static_cast<int>(node->args.size())));
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
        RUNTIME_CALL(envSet, env, fromInt(node->name->symbol), result);
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



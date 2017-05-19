#pragma once
#include "llvm.h"
#include "ast.h"
#include "runtime.h"
#include "types.h"

namespace rift {

class RiftModule;

class Compiler : public Visitor {
public:

    /** Returns the llvm's context for the compiler.
     */
    static llvm::LLVMContext & context() {
        static llvm::LLVMContext context;
        return context;
    }

    int compile(ast::Fun * f);

    Compiler();

    ~Compiler() {
        if (cur.b)
            delete cur.b;
    }

#if VERSION > 0

#define FUN_PURE(NAME, SIGNATURE) static llvm::Function * NAME(llvm::Module * m);
#define FUN(NAME, SIGNATURE) static llvm::Function * NAME(llvm::Module * m);
RUNTIME_FUNCTIONS
#undef FUN_PURE
#undef FUN

private:

    /** Create Value from double scalar .  */
    llvm::Value * fromDouble(double value) {
        return llvm::ConstantFP::get(context(), llvm::APFloat(value));
    }

    /** Create Value from integer. */
    llvm::Value * fromInt(int value) {
        return llvm::ConstantInt::get(context(), llvm::APInt(32, value));
    }

// TODO this should be private when we are done with the old compiler
public:

    void visit(ast::Exp * node) override;
    void visit(ast::Num * node) override;
    void visit(ast::Str * node) override;
    void visit(ast::Var * node) override;
    void visit(ast::Seq * node) override;
    void visit(ast::Fun * node) override;
    void visit(ast::BinExp * node) override;
    void visit(ast::UserCall * node) override;
    void visit(ast::LengthCall * node) override;
    void visit(ast::TypeCall * node) override;
    void visit(ast::EvalCall * node)  override;
    void visit(ast::CCall * node)  override;
    void visit(ast::Index * node) override;
    void visit(ast::SimpleAssignment * node) override;
    void visit(ast::IndexAssignment * node) override;
    void visit(ast::IfElse * node) override;
    void visit(ast::WhileLoop * node) override;
#endif //VERSION

private:

    friend class JIT;

    llvm::Value * result;

    unique_ptr<llvm::Module> m;

    struct FunctionContext {
        FunctionContext() : f(nullptr), env(nullptr), b(nullptr) {}
        FunctionContext(string name, llvm::Module* m);

        llvm::Function * f;
        llvm::Value * env;
        llvm::IRBuilder<> * b;

        void restore(const FunctionContext& other) {
            if (b) delete b;
            f = other.f;
            env = other.env;
            b = other.b;
        }
    };

    FunctionContext cur;
};






}

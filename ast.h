#pragma once
#ifndef AST_H
#define AST_H
#include <iostream>
#include <ciso646>

#include "lexer.h"

namespace rift {

typedef int Symbol;

class Visitor;

namespace ast {
    /** Base class for all expressions in rift.  */
    class Exp {
    public:
        virtual ~Exp() {}
        virtual void accept(Visitor * v) = 0;
        void print(std::ostream & s);
    };

    /** Double scalar literal.  */
    class Num : public Exp {
    public:
        Num(double value): value(value) {}
        void accept(Visitor * v) override;
        double value;
    };


    /** Character literal. */
    class Str : public Exp {
    public:
        Str(unsigned index): index(index) {}
        void accept(Visitor * v) override;
        std::string const & value() const;
        unsigned index;
    };

    /** Variable read.  */
    class Var : public Exp  {
    public:
        Var(Symbol symbol): symbol(symbol) {}
        void accept(Visitor * v) override;
        std::string const & value() const;
        unsigned symbol;
    };

    /** Block of statements.  */
    class Seq : public Exp {
    public:
        ~Seq() override  {
            for (Exp * e : body) delete e;
        }
        void accept(Visitor * v) override;
        std::vector<Exp*> body;
    };

    /** Function definition.  */
    class Fun: public Exp {
    public:
        Fun(): body(nullptr) {}
        Fun(ast::Exp * body):
            body(dynamic_cast<ast::Seq *>(body)) {
        }
        ~Fun() {
            for (Var * v : args) delete v;
            delete body;
        }
        void accept(Visitor * v) override;
        Seq * body;
        std::vector<Var *> args;
    };

    /** Binary expression. */
    class BinExp : public Exp {
    public:
        enum class Type { add, sub, mul, div, eq, neq, lt, gt, dot };

        BinExp(Exp * lhs, Exp * rhs, Type t): lhs(lhs), rhs(rhs), type(t) { }
        ~BinExp() {   delete lhs; delete rhs; }
        void accept(Visitor * v) override;
        Exp * lhs;
        Exp * rhs;
        Type type;
    };

    /** Function call. Stores a vector of argument exps.  */
    class Call: public Exp {
    public:
        ~Call() {  for (Exp * e : args) delete e; }
        void accept(Visitor * v) override;
	std::vector<Exp*> args;
    };

    /** Call to user defined function */
    class UserCall : public Call {
    public:
        UserCall(Exp * name): name(name) { }
        ~UserCall() { delete name; }
        void accept(Visitor * v) override;
        Exp * name;
    };

    /** Call to Rift runtime  */
    class SpecialCall : public Call {
    public:
        void accept(Visitor * v) override;
    };

    /** Call to c().   */
    class CCall : public SpecialCall {
    public:
        void accept(Visitor * v) override;
    };

    /** Call to eval(). */
    class EvalCall : public SpecialCall {
    public:
        EvalCall(ast::Exp * arg) {
            args.push_back(arg);
        }
        void accept(Visitor * v) override;
    };

    /** Call to length().  */
    class LengthCall : public SpecialCall {
    public:
        LengthCall(ast::Exp * arg) {
            args.push_back(arg);
        }
        void accept(Visitor * v) override;
    };

    /** Call to a type().  */
    class TypeCall : public SpecialCall {
    public:
        TypeCall(ast::Exp * arg) {
            args.push_back(arg);
        }
        void accept(Visitor * v) override;
    };

    /** Indexed read.  */
    class Index : public Exp {
    public:
        Index(Exp * name):
            name(name),
            index(nullptr) {
        }
        ~Index() {
            delete name;
            delete index;
        }
        void accept(Visitor * v) override;
        Exp * name;
        Exp * index;
    };

    /** Assignment */
    class Assignment : public Exp {
    public:
        void accept(Visitor * v) override;
    };

    /** Assignment  to a symbol.  */
    class SimpleAssignment : public Assignment {
    public:
        SimpleAssignment(Var * name):
            name(name),
            rhs(nullptr) {
        }
        ~SimpleAssignment() {
            delete name;
            delete rhs;
        }
        void accept(Visitor * v) override;
        Var * name;
        Exp * rhs;
    };

    /** Assignment to an indexed. */
    class IndexAssignment : public Assignment {
    public:
        IndexAssignment(Index * index):
            index(index),
            rhs(nullptr) {
        }
        ~IndexAssignment() override {
            delete index;
            delete rhs;
        }
        void accept(Visitor * v) override;
        Index * index;
        Exp * rhs;
    };

    /** Conditional   */
    class IfElse : public Exp {
    public:
        IfElse(Exp * guard):
            guard(guard),
            ifClause(nullptr),
            elseClause(nullptr) {
        }
        ~IfElse() {
            delete guard;
            delete ifClause;
            delete elseClause;
        }
        void accept(Visitor * v) override;
        Exp * guard;
        Exp * ifClause;
        Exp * elseClause;
    };

    /** While loop.  */
    class WhileLoop : public Exp {
    public:
        WhileLoop(Exp * guard):
            guard(guard),
            body(nullptr) {
        }
        ~WhileLoop() override {
            delete guard;
            delete body;
        }
        void accept(Visitor * v) override;
        Exp * guard;
        Seq * body;
    };

} // namespace ast

class Visitor {
public:
    virtual void visit(ast::Exp * n) {}
    virtual void visit(ast::Num * n) { visit(static_cast<ast::Exp*>(n)); }
    virtual void visit(ast::Str * n) { visit(static_cast<ast::Exp*>(n)); }
    virtual void visit(ast::Var * n) { visit(static_cast<ast::Exp*>(n)); }
    virtual void visit(ast::Seq * n) { visit(static_cast<ast::Exp*>(n)); }
    virtual void visit(ast::Fun * n) { visit(static_cast<ast::Exp*>(n)); }
    virtual void visit(ast::BinExp * n)      { visit(static_cast<ast::Exp*>(n)); }
    virtual void visit(ast::Call * n)        { visit(static_cast<ast::Exp*>(n)); }
    virtual void visit(ast::UserCall * n)    { visit(static_cast<ast::Call*>(n)); }
    virtual void visit(ast::SpecialCall * n) { visit(static_cast<ast::Call*>(n)); }
    virtual void visit(ast::CCall * n)       { visit(static_cast<ast::SpecialCall*>(n)); }
    virtual void visit(ast::EvalCall * n)    { visit(static_cast<ast::SpecialCall*>(n)); }
    virtual void visit(ast::TypeCall * n)    { visit(static_cast<ast::SpecialCall*>(n)); }
    virtual void visit(ast::LengthCall * n)  { visit(static_cast<ast::SpecialCall*>(n)); }
    virtual void visit(ast::Index * n)       { visit(static_cast<ast::Exp*>(n)); }
    virtual void visit(ast::Assignment * n)  { visit(static_cast<ast::Exp*>(n)); }
    virtual void visit(ast::SimpleAssignment * n) { visit(static_cast<ast::Assignment*>(n)); }
    virtual void visit(ast::IndexAssignment * n) { visit(static_cast<ast::Assignment*>(n)); }
    virtual void visit(ast::IfElse * n)      { visit(static_cast<ast::Exp*>(n)); }
    virtual void visit(ast::WhileLoop * n)   { visit(static_cast<ast::Exp*>(n)); }
};

} // namespace rift2
#endif // AST_H


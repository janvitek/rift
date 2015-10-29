#ifndef AST_H
#define AST_H


#include <iostream>

#include "lexer.h"

namespace rift {

class Visitor;

namespace ast {
    class Exp {
    public:
        virtual ~Exp() {}
        virtual void accept(Visitor * v) = 0;
        void print(std::ostream & s);
    };


    class Num : public Exp {
    public:
        Num(double value):
            value_(value) {}

        void accept(Visitor * v) override;

        double value() const {
            return value_;
        }


    private:
        double value_;
    };


    class Str : public Exp {
    public:
        Str(unsigned index):
            index_(index) {}
        void accept(Visitor * v) override;
        std::string const & value() const {
            return Lexer::getPoolObject(index_);
        }

        unsigned index() const {
            return index_;
        }

    private:
        unsigned index_;
    };

    class Var : public Exp  {
    public:
        Var(unsigned index):
            index_(index) {}
        void accept(Visitor * v) override;
        std::string const & value() const {
            return Lexer::getPoolObject(index_);
        }
        unsigned index() const {
            return index_;
        }
    private:
        unsigned index_;
    };

    class Seq : public Exp {
    public:
        ~Seq() override  {
            for (Exp * e : body_)
                delete e;
        }

        void accept(Visitor * v) override;

        void push_back(Exp * exp) {
            body_.push_back(exp);
        }

        std::vector<Exp *>::iterator begin() {
            return body_.begin();
        }

        std::vector<Exp *>::iterator end() {
            return body_.end();
        }

        unsigned size() const {
            return body_.size();
        }

    private:
        std::vector<Exp*> body_;
    };

    class Fun: public Exp {
    public:

        Fun():
            body_(nullptr) {
        }

        Fun(ast::Exp * body):
            body_(dynamic_cast<ast::Seq *>(body)) {
        }

        ~Fun() {
            for (Var * v : args_)
                delete v;
            delete body_;
        }

        void accept(Visitor * v) override;

        void setBody(Seq * body) {
            delete body_;
            body_ = body;
        }

        Seq * body() {
            return body_;
        }

        std::vector<Var *>::iterator begin() {
            return args_.begin();
        }

        std::vector<Var *>::iterator end() {
            return args_.end();
        }

        void addArgument(Var * arg) {
            args_.push_back(arg);
        }

        unsigned argumentCount() const {
            return args_.size();
        }

    private:
        Seq * body_;
        std::vector<Var *> args_;
    };

    class BinExp : public Exp {
    public:
        BinExp(Exp * lhs, Exp * rhs, Token::Type t):
            lhs_(lhs),
            rhs_(rhs),
            type_(t) {}
        ~BinExp() {
            delete lhs_;
            delete rhs_;
        }

        void accept(Visitor * v) override;

        Exp * lhs() const {
            return lhs_;
        }

        Exp * rhs() const {
            return rhs_;
        }

        Token::Type type() const {
            return type_;
        }

    private:
        Exp * lhs_;
        Exp * rhs_;
        Token::Type type_;
    };

    class Call: public Exp {
    public:

        ~Call() {
            for (Exp * e : args_)
                delete e;
        }

        void accept(Visitor * v) override;

        std::vector<Exp *>::iterator begin() {
            return args_.begin();
        }

        std::vector<Exp *>::iterator end() {
            return args_.end();
        }

        Call * addArgument(Exp * arg) {
            args_.push_back(arg);
            return this;
        }

        unsigned argumentCount() {
            return args_.size();
        }

        Exp * operator[](unsigned index) {
            return args_[index];
        }


    private:

        std::vector<Exp*> args_;

    };

    class UserCall : public Call {
    public:
        UserCall(Var * name):
            name_(name) {
        }
        ~UserCall() {
            delete name_;
        }

        void accept(Visitor * v) override;

        Var * name() {
            return name_;
        }

    private:
        Var * name_;
    };

    class SpecialCall : public Call {
    public:
        SpecialCall(Token::Type target):
            target_(target) {
        }

        void accept(Visitor * v) override;

        Token::Type target() const {
            return target_;
        }

    private:
        Token::Type target_;
    };

    class Index : public Exp {

    public:
        Index(Var * name):
            name_(name),
            index_(nullptr) {
        }

        ~Index() {
            delete name_;
            delete index_;
        }

        void accept(Visitor * v) override;

        Var * name() {
            return name_;
        }

        Exp * index() {
            return index_;
        }

        void setIndex(Exp * index) {
            delete index_;
            index_ = index;
        }

    private:
        Var * name_;
        Exp * index_;
    };

    class Assignment : public Exp {
    public:
        void accept(Visitor * v) override;

    };

    class SimpleAssignment : public Assignment {
    public:

        SimpleAssignment(Var * name):
            name_(name),
            rhs_(nullptr) {
        }

        ~SimpleAssignment() {
            delete name_;
            delete rhs_;
        }

        void accept(Visitor * v) override;

        Var * name() const {
            return name_;
        }

        Exp * rhs() const {
            return rhs_;
        }

        void setRhs(Exp * rhs) {
            delete rhs_;
            rhs_ = rhs;
        }

    private:
        Var * name_;
        Exp * rhs_;

    };

    class IndexAssignment : public Assignment {
    public:

        IndexAssignment(Index * index):
            index_(index),
            rhs_(nullptr) {
        }

        ~IndexAssignment() override {
            delete index_;
            delete rhs_;
        }

        void accept(Visitor * v) override;

        Index * index() const {
            return index_;
        }

        Exp * rhs() const {
            return rhs_;
        }

        void setRhs(Exp * rhs) {
            delete rhs_;
            rhs_ = rhs;
        }

    private:
        Index * index_;
        Exp * rhs_;

    };

    class IfElse : public Exp {
    public:
        IfElse(Exp * guard):
            guard_(guard),
            ifClause_(nullptr),
            elseClause_(nullptr) {
        }

        ~IfElse() {
            delete guard_;
            delete ifClause_;
            delete elseClause_;
        }

        void accept(Visitor * v) override;

        Exp * guard() const {
            return guard_;
        }

        Exp * ifClause() const {
            return ifClause_;
        }

        void setIfClause(Exp * value) {
            delete ifClause_;
            ifClause_ = value;
        }

        Exp * elseClause() const {
            return elseClause_;
        }

        void setElseClause(Exp * value) {
            delete elseClause_;
            elseClause_ = value;
        }


    private:
        Exp * guard_;
        Exp * ifClause_;
        Exp * elseClause_;
    };

    class WhileLoop : public Exp {
    public:
        WhileLoop(Exp * guard):
            guard_(guard),
            body_(nullptr) {
        }

        ~WhileLoop() override {
            delete guard_;
            delete body_;
        }

        void accept(Visitor * v) override;

        Exp * guard() const {
            return guard_;
        }

        Seq * body() const {
            return body_;
        }

        void setBody(Seq * value) {
            delete body_;
            body_ = value;
        }

    private:
        Exp * guard_;
        Seq * body_;
    };




} // namespace ast

class Visitor {
public:
    virtual void visit(ast::Exp * node) {}
    virtual void visit(ast::Num * node) { visit(static_cast<ast::Exp*>(node)); }
    virtual void visit(ast::Str * node) { visit(static_cast<ast::Exp*>(node)); }
    virtual void visit(ast::Var * node) { visit(static_cast<ast::Exp*>(node)); }
    virtual void visit(ast::Seq * node) { visit(static_cast<ast::Exp*>(node)); }
    virtual void visit(ast::Fun * node) { visit(static_cast<ast::Exp*>(node)); }
    virtual void visit(ast::BinExp * node) { visit(static_cast<ast::Exp*>(node)); }
    virtual void visit(ast::Call * node) { visit(static_cast<ast::Exp*>(node)); }
    virtual void visit(ast::UserCall * node) { visit(static_cast<ast::Call*>(node)); }
    virtual void visit(ast::SpecialCall * node) { visit(static_cast<ast::Call*>(node)); }
    virtual void visit(ast::Index * node) { visit(static_cast<ast::Exp*>(node)); }
    virtual void visit(ast::Assignment * node) { visit(static_cast<ast::Exp*>(node)); }
    virtual void visit(ast::SimpleAssignment * node) { visit(static_cast<ast::Assignment*>(node)); }
    virtual void visit(ast::IndexAssignment * node) { visit(static_cast<ast::Assignment*>(node)); }
    virtual void visit(ast::IfElse * node) { visit(static_cast<ast::Exp*>(node)); }
    virtual void visit(ast::WhileLoop * node) { visit(static_cast<ast::Exp*>(node)); }
};


} // namespace rift2

#endif // AST_H


#pragma once

#include "ast.h"
#include <iostream>


namespace rift {
namespace ast {

/** Printer is a visitor over the abstract syntax tree.
    The vist methods perform actions for the corresponding
    nodes.
 
    Usage:  Printer::print(exp);
 */
class Printer: public Visitor {
public:
    void visit(Exp * n) override {
        s << "???";
    }
    void visit(Num * n) override {
        s << n->value;
    }
    void visit(Str * n) override {
        s << '"' << n->value() << '"';
    }
    void visit(Var * n) override {
        s << n->value();
    }
    void visit(Seq * n) override {
        for (Exp * e : n->body) {
            e->accept(this);
            s << "\n";
        }
    }
    void visit(Fun * n) override {
        s << "function(" ;
        for (Var * v : n->args) {
            v->accept(this);
            s << ", ";
        }
        s << ") {\n";
        n->body->accept(this);
        s << "}";
    }
    void visit(BinExp * n) override {
        n->lhs->accept(this);
        s << " ";
        switch (n->type) {
        case BinExp::Type::add: s << "+"; break;
        case BinExp::Type::sub: s << "-"; break;
        case BinExp::Type::mul: s << "*"; break;
        case BinExp::Type::div: s << "/"; break;
        case BinExp::Type::eq:  s << "=="; break;
        case BinExp::Type::neq: s << "!="; break;
        case BinExp::Type::lt:  s << "<"; break;
        case BinExp::Type::gt:  s << ">";  break;
        default:                     s << "?";
        }
        s << " ";
        n->rhs->accept(this);
    }
    void printArgs(Call * n) {
#if VERSION <= 1
        // TODO
        assert(false);
#endif //VERSION
#if VERSION > 1
        s << "(";
        for (Exp * v : n->args) {
            v->accept(this);
            s << ", ";
        }
        s << ")";
#endif //VERSION
    }
    void visit(UserCall * n) override {
#if VERSION <= 1
        // TODO
        assert(false);
#endif //VERSION
#if VERSION > 1
        n->name->accept(this);
        printArgs(n);
#endif //VERSION
    }
    void visit(CCall * n) override {
        s << "c";
        printArgs(n);
    }
    void visit(EvalCall * n) override {
        s << "eval";
        printArgs(n);
    }
    void visit(TypeCall * n) override {
        s << "type";
        printArgs(n);
    }
    void visit(LengthCall * n) override {
        s << "length";
        printArgs(n);
    }
    void visit(Index * n) override {
        n->name->accept(this);
        s << "[";
        n->index->accept(this);
        s << "]";
    }
    void visit(SimpleAssignment * n) override {
        n->name->accept(this);
        s << " <- ";
        n->rhs->accept(this);
    }
    void visit(IndexAssignment * n) override {
        n->index->accept(this);
        s << " <- ";
        n->rhs->accept(this);
    }
    void visit(IfElse * n) override {
        s << "if (";
        n->guard->accept(this);
        s << ") ";
        n->ifClause->accept(this);
        if (n->elseClause != nullptr) {
            s << " else ";
            n->elseClause ->accept(this);
        }
    }
    void visit(WhileLoop * n) override {
        s << "while (";
        n->guard->accept(this);
        s << ") ";
        n->body->accept(this);
    }

    /** Print the expression to stdout. */
    static void print(Exp * exp) {
        return print(exp, cout);
    }

private:
    Printer(ostream & s): s(s) { }

    ostream & s;

    /** Print the expression on the outputstream.   */
    static void print(Exp * exp, ostream & s) {
        Printer p(s);
        exp->accept(&p);
    }


};
}
}

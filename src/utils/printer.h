#pragma once

#include "ast.h"

namespace rift {

class Printer: public Visitor {
public:
    static void print(ast::Exp * exp, std::ostream & s);
    static void print(ast::Exp * exp);

    void visit(ast::Exp * node) override {
        s << "???";
    }
    void visit(ast::Num * node) override {
        s << node->value;
    }
    void visit(ast::Str * node) override {
        s << '"' << node->value() << '"';
    }
    void visit(ast::Var * node) override {
        s << node->value();
    }
    void visit(ast::Seq * node) override {
        for (ast::Exp * e : node->body) {
            e->accept(this);
            s << "\n";
        }
    }
    void visit(ast::Fun * node) override {
        s << "function(" ;
        for (ast::Var * v : node->args) {
            v->accept(this);
            s << ", ";
        }
        s << ") {\n";
        node->body->accept(this);
        s << "}";

    }
    void visit(ast::BinExp * node) override {
        node->lhs->accept(this);
        s << " ";
        switch (node->type) {
        case ast::BinExp::Type::add:   s << "+"; break;
        case ast::BinExp::Type::sub:   s << "-"; break;
        case ast::BinExp::Type::mul:   s << "*"; break;
        case ast::BinExp::Type::div:   s << "/"; break;
        case ast::BinExp::Type::eq:    s << "=="; break;
        case ast::BinExp::Type::neq:   s << "!="; break;
        case ast::BinExp::Type::lt:    s << "<"; break;
        case ast::BinExp::Type::gt:    s << ">";  break;
        default:                       s << "?";
        }
        s << " ";
        node->rhs->accept(this);
    }
    void printArgs(ast::Call * node) {
        s << "(";
        for (ast::Exp * v : node->args) {
            v->accept(this);
            s << ", ";
        }
        s << ")";
    }
    void visit(ast::UserCall * node) override {
        node->name->accept(this);
        printArgs(node);
    }
    void visit(ast::CCall * node) override {
        s << "c";
        printArgs(node);
    }
    void visit(ast::EvalCall * node) override {
        s << "eval";
        printArgs(node);
    }
    void visit(ast::TypeCall * node) override {
        s << "type";
        printArgs(node);
    }
    void visit(ast::LengthCall * node) override {
        s << "length";
        printArgs(node);
    }
    void visit(ast::Index * node) override {
        node->name->accept(this);
        s << "[";
        node->index->accept(this);
        s << "]";
    }
    void visit(ast::SimpleAssignment * node) override {
        node->name->accept(this);
        s << " <- ";
        node->rhs->accept(this);
    }
    void visit(ast::IndexAssignment * node) override {
        node->index->accept(this);
        s << " <- ";
        node->rhs->accept(this);
    }
    void visit(ast::IfElse * node) override {
        s << "if (";
        node->guard->accept(this);
        s << ") ";
        node->ifClause->accept(this);
        if (node->elseClause != nullptr) {
            s << " else ";
            node->elseClause ->accept(this);
        }
    }
    void visit(ast::WhileLoop * node) override {
        s << "while (";
        node->guard->accept(this);
        s << ") ";
        node->body->accept(this);
    }

private:
    Printer(std::ostream & s):
        s(s) {
    }

    std::ostream & s;

};

}

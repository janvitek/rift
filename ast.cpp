#include "ast.h"
#include "pool.h"

namespace rift {
namespace ast {

std::string const & Str::value() const {
    return Pool::getPoolObject(index);
}

std::string const & Var::value() const {
    return Pool::getPoolObject(symbol);
}

class Printer: public Visitor {
public:
    Printer(std::ostream & s):
        s(s) {
    }
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
        for (Exp * e : node->body) {
            e->accept(this);
            s << "\n";
        }
    }
    void visit(ast::Fun * node) override {
        s << "function(" ;
        for (Var * v : node->args) {
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
        case ast::BinExp::Type::dot:   s << "%*%";  break;
        default:                       s << "?";
        }
        s << " ";
        node->rhs->accept(this);
    }
    void visit(ast::Call * node) override {
        s << "(";
        for (Exp * v : node->args) {
            v->accept(this);
            s << ", ";
        }
        s << ")";
    }
    void visit(ast::UserCall * node) override {
        node->name->accept(this);
        visit(static_cast<ast::Call*>(node));
    }
    void visit(ast::SpecialCall * node) override {
        visit(static_cast<ast::Call*>(node));
    }
    void visit(ast::CCall * node) override {
        s << "c";
        visit(static_cast<ast::SpecialCall*>(node));
    }
    void visit(ast::EvalCall * node) override {
        s << "eval";
        visit(static_cast<ast::SpecialCall*>(node));
    }
    void visit(ast::TypeCall * node) override {
        s << "type";
        visit(static_cast<ast::SpecialCall*>(node));
    }
    void visit(ast::LengthCall * node) override {
        s << "length";
        visit(static_cast<ast::SpecialCall*>(node));
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
    std::ostream & s;

};

void Exp::print(std::ostream & s) {
    Printer p(s);
    this->accept(&p);
}

void Num::accept(Visitor * v)     { v->visit(this); }
void Str::accept(Visitor * v)     { v->visit(this); }
void Var::accept(Visitor * v)     { v->visit(this); }
void Seq::accept(Visitor * v)     { v->visit(this); }
void Fun::accept(Visitor * v)     { v->visit(this); }
void BinExp::accept(Visitor * v)  { v->visit(this); }
void Call::accept(Visitor * v)    { v->visit(this); }
void UserCall::accept(Visitor * v) { v->visit(this); }
void SpecialCall::accept(Visitor * v) { v->visit(this); }
void CCall::accept(Visitor * v)   { v->visit(this); }
void EvalCall::accept(Visitor * v) { v->visit(this); }
void TypeCall::accept(Visitor * v) { v->visit(this); }
void LengthCall::accept(Visitor * v) { v->visit(this); }
void Index::accept(Visitor * v)    { v->visit(this); }
void Assignment::accept(Visitor * v) { v->visit(this); }
void SimpleAssignment::accept(Visitor * v) { v->visit(this); }
void IndexAssignment::accept(Visitor * v) { v->visit(this); }
void IfElse::accept(Visitor * v)  { v->visit(this); }
void WhileLoop::accept(Visitor * v) { v->visit(this); }
}
}

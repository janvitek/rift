#include "ast.h"


namespace rift {
namespace ast {

class Printer: public Visitor {
public:
    Printer(std::ostream & s):
        s(s) {
    }

    void visit(ast::Exp * node) override {
        s << "???";
    }

    void visit(ast::Num * node) override {
        s << node->value();
    }
    void visit(ast::Str * node) override {
        s << '"' << node->value() << '"';
    }
    void visit(ast::Var * node) override {
        s << node->value();
    }
    void visit(ast::Seq * node) override {
        for (Exp * e : *node) {
            e->accept(this);
            s << "\n";
        }
    }
    void visit(ast::Fun * node) override {
        s << "function(" ;
        for (Var * v : * node) {
            v->accept(this);
            s << ", ";
        }
        s << ")\n";
        node->body()->accept(this);

    }
    void visit(ast::BinExp * node) override {
        node->lhs()->accept(this);
        s << " " << Token::typeToStr(node->type()) << " ";
        node->rhs()->accept(this);
    }

    void visit(ast::Call * node) override {
        for (Exp * v : * node) {
            v->accept(this);
            s << ", ";
        }
        s << ")";
    }
    void visit(ast::UserCall * node) override {
        node->name()->accept(this);
        visit(static_cast<ast::Call*>(node));
    }

    void visit(ast::SpecialCall * node) override {
        switch (node->target()) {
        case Token::Type::kwC:
            s << "c";
            break;
        case Token::Type::kwLength:
            s << "length";
            break;
        case Token::Type::kwType:
            s << "type";
            break;
        case Token::Type::kwEval:
            s << "eval";
            break;
        }
        visit(static_cast<ast::Call*>(node));
    }

    void visit(ast::Index * node) override {
        node->name()->accept(this);
        s << "[";
        node->index()->accept(this);
        s << "]";
    }
    void visit(ast::SimpleAssignment * node) override {
        node->name()->accept(this);
        s << " <- ";
        node->rhs()->accept(this);
    }
    void visit(ast::IndexAssignment * node) override {
        node->index()->accept(this);
        s << " <- ";
        node->rhs()->accept(this);
    }
    void visit(ast::IfElse * node) override {
        s << "if (";
        node->guard()->accept(this);
        s << ") ";
        node->ifClause()->accept(this);
        if (node->elseClause() != nullptr) {
            s << " else ";
            node->elseClause() ->accept(this);
        }
    }
    void visit(ast::WhileLoop * node) override {
        s << "while (";
        node->guard()->accept(this);
        s << ") ";
        node->body()->accept(this);
    }

private:
    std::ostream & s;

};

void Exp::print(std::ostream & s) {
    Printer p(s);
    this->accept(&p);
}

void Num::accept(Visitor * v) {
    v->visit(this);
}
void Str::accept(Visitor * v) {
    v->visit(this);
}
void Var::accept(Visitor * v) {
    v->visit(this);
}
void Seq::accept(Visitor * v) {
    v->visit(this);
}
void Fun::accept(Visitor * v) {
    v->visit(this);
}
void BinExp::accept(Visitor * v) {
    v->visit(this);
}
void Call::accept(Visitor * v) {
    v->visit(this);
}
void UserCall::accept(Visitor * v) {
    v->visit(this);
}
void SpecialCall::accept(Visitor * v) {
    v->visit(this);
}
void Index::accept(Visitor * v) {
    v->visit(this);
}
void Assignment::accept(Visitor * v) {
    v->visit(this);
}
void SimpleAssignment::accept(Visitor * v) {
    v->visit(this);
}
void IndexAssignment::accept(Visitor * v) {
    v->visit(this);
}
void IfElse::accept(Visitor * v) {
    v->visit(this);
}
void WhileLoop::accept(Visitor * v) {
    v->visit(this);
}

}
}

#ifndef H_PARSE
#define H_PARSE

#include <vector>
#include <iostream>
#include <unordered_map>
#include "lex.h"

namespace rift {

class Visitor;

class Exp {
public:
    virtual ~Exp() {
    }
    virtual void accept(Visitor* v) = 0;
    void print();
};

class Num : public Exp {
public:
    double value;
    Num(double v)
        : value(v) {
    }
    void accept(Visitor* v) override;
};

class Str : public Exp {
public:
    std::string value;
    Str(std::string const& v)
        : value(v) {
    }
    void accept(Visitor* v) override;
};

class Var : public Exp {
public:
    std::string value;
    int index;
    Var(std::string const& v, int index)
        : value(v), index(index) {
    }
    void accept(Visitor* v) override;
};

class Seq : public Exp {
public:
    std::vector<Exp*> exps;
    Seq() = default;
    void push_back(Exp* e) {
        exps.push_back(e);
    }

    void accept(Visitor* v) override;
    ~Seq() {
        for (Exp* e : exps)
            delete e;
    }
};

class Fun : public Exp {
public:
    std::vector<Var*> params;
    Seq* body;
    Fun() = default;
    void addParam(Var* v) {
        params.push_back(v);
    }

    void setBody(Seq* b) {
        body = b;
    }

    void accept(Visitor* v) override;
    ~Fun() {
        for (Var* v : params)
            delete v;
        delete body;
    }
};

class BinExp : public Exp {
public:
    Exp* left, *right;
    Token op;
    BinExp(Exp* l, Token op, Exp* r)
        : left(l)
        , op(op)
        , right(r) {
    }
    void accept(Visitor* v) override;
    ~BinExp() {
        delete left;
        delete right;
    }
};

class Call : public Exp {
public:
    Var* name;
    std::vector<Exp*> args;
    Call(Var* n)
        : name(n) {
    }
    void addArg(Exp* a) {
        args.push_back(a);
    }

    void accept(Visitor* v) override;
    ~Call() {
        delete name;
        for (Exp* e : args)
            delete e;
    }
};

class Idx : public Exp {
public:
    Var* name;
    Exp* body;
    Idx(Var* n, Exp* b)
        : name(n)
        , body(b) {
    }
    void accept(Visitor* v) override;
    ~Idx() {
        delete name;
        delete body;
    }
};

class Assign : public Exp {};

class SimpleAssign : public Assign {
public:
    Var* name;
    Exp* rhs;
    SimpleAssign(Var* v, Exp* e)
        : name(v)
        , rhs(e) {
    }
    void accept(Visitor* v) override;
    ~SimpleAssign() {
        delete name;
        delete rhs;
    }
};

class IdxAssign : public Assign {
public:
    Idx* lhs;
    Exp* rhs;
    IdxAssign(Idx* e, Exp* e2)
        : lhs(e)
        , rhs(e2) {
    }
    void accept(Visitor* v) override;
    ~IdxAssign() {
        delete lhs;
        delete rhs;
    }
};

class IfElse : public Exp {
public:
    Exp* guard;
    Seq* ifclause, *elseclause;
    IfElse(Exp* g, Seq* i, Seq* e)
        : guard(g)
        , ifclause(i)
        , elseclause(e) {
    }
    void accept(Visitor* v) override;
    ~IfElse() {
        delete guard;
        delete ifclause;
        delete elseclause;
    }
};

class Visitor {
public:
    virtual void visit(Num* x) = 0;
    virtual void visit(Str* x) = 0;
    virtual void visit(Var* x) = 0;
    virtual void visit(Fun* x) = 0;
    virtual void visit(BinExp* x) = 0;
    virtual void visit(Seq* x) = 0;
    virtual void visit(Call* x) = 0;
    virtual void visit(Idx* x) = 0;
    virtual void visit(SimpleAssign* x) = 0;
    virtual void visit(IdxAssign* x) = 0;
    virtual void visit(IfElse* x) = 0;
};

class Parser {
    File file;
    std::vector<Token> tokens;
    std::vector<std::string> strings;
    std::vector<double> doubles;
    std::unordered_map<std::string, int> symbols;
    int cursor;
    Seq* parseSequence();
    Exp* parseExp();
    Exp* parseBinExp(Exp* e);
    Exp* parsePrimaryExp();

public:
    Parser(File f)
        : file(f) {
        Token tok;
        int count = 0;
        cursor = 0;
        while ((tok = file.next()) != END) {
            count++;
            tokens.push_back(tok);
            if (tok == IDENT) {
                strings.push_back(file.ident_or_string);
                auto idx = symbols.find(file.ident_or_string);
                if (idx == symbols.end()) {
                    int i = symbols.size();
                    symbols[file.ident_or_string] = i;
                    doubles.push_back(i);
                } else {
                    doubles.push_back(idx->second);
                }
            } else {
                strings.push_back(file.ident_or_string);
                doubles.push_back((tok == NUM) ? file.number : 0.0);
            }
        }
        tokens.push_back(END);
    }

    Token token();
    std::string const & string_value();
    double double_value();
    void advance();
    Seq* parse();
};

} // namespace rift
#endif

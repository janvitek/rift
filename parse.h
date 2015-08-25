#ifndef H_PARSE
#define H_PARSE

#include <vector>
#include <iostream>
#include <unordered_map>
#include "lex.h"

using namespace std;

using std::endl;
using std::string;

namespace rift {

class Visitor;

class Exp {
public:
  virtual ~Exp() {}
  virtual void accept(Visitor*  v) = 0;
  void print();
};

class Num : public Exp {
public:
  double value;
  Num(double v) : value(v) {}
  void accept(Visitor* v) override;
};

class Str : public Exp {
public:
  string* value;
  Str(string* v) : value(v) {}
  void accept(Visitor* v) override;
};

class Var : public Exp {
public:
  string* value;
  Var(string* v) : value(v) {}
  void accept(Visitor* v) override;
  ~Var() {
      delete value;
  }
};

class Seq : public Exp {
public:
  std::vector<Exp*> * exps;
  Seq(std::vector<Exp*>* v) : exps(v) {}
  void accept(Visitor* v) override;
  ~Seq() {
    for (Exp * e : *exps)
       delete e;
    delete exps;
  }
};


class Fun : public Exp {
public:
  std::vector<Var*> *params;
  Seq* body;
  Fun(std::vector<Var*> *params, Seq *body) : params(params), body(body) {}
  void accept(Visitor* v) override;
  ~Fun() {
      for (Var * v : *params)
          delete v;
    delete params;
    delete body;
  }
};

class BinExp : public Exp {
public:
  Exp *left, *right;
  Token op;
  BinExp(Exp* l, Token op, Exp* r) : left(l), op(op), right(r) {}
  void accept(Visitor* v) override;
  ~BinExp() {
    delete left; 
    delete right;
  }
};

class Call : public Exp {
public:
  Var* name;
  std::vector<Exp*>* args;
  Call(Var* n, std::vector<Exp*>* a) : name(n), args(a) {}
  void accept(Visitor* v) override;
  ~Call() {
    delete name;
      for (Exp * e : *args)
          delete e;
    delete args;
  }
};

class Idx : public Exp {
public:
  Var *name;
  Exp *body;
  Idx(Var* n, Exp* b) : name(n), body(b) {} 
  void accept(Visitor* v) override;
 ~Idx() {
    delete name;
    delete body;
  }
};


class Assign : public Exp { };

class SimpleAssign : public Assign {
public:
  Var* name;
  Exp* rhs;
 SimpleAssign(Var* v, Exp* e) : name(v), rhs(e) {}
  void accept(Visitor* v) override;
  ~SimpleAssign() {
    delete name;
    delete rhs;
  }
};

class IdxAssign : public Assign {
public:
  Idx *lhs;
  Exp *rhs;
 IdxAssign(Idx* e, Exp* e2) : lhs(e), rhs(e2) {}
  void accept(Visitor* v) override;
  ~IdxAssign() {
    delete lhs;
    delete rhs;
  }
};

class IfElse : public Exp {
public:
  Exp *guard;
  Seq *ifclause, *elseclause;
 IfElse(Exp* g, Seq* i, Seq* e) : guard(g), ifclause(i), elseclause(e) {}
  void accept(Visitor* v) override;
  ~IfElse() {
    delete guard;
    delete ifclause;
    delete elseclause;
  }
};


class Visitor {
 public:
  virtual void visit(Num * x)          =0;
  virtual void visit(Str * x)          =0;
  virtual void visit(Var * x)          =0;
  virtual void visit(Fun * x)          =0;
  virtual void visit(BinExp * x)       =0;
  virtual void visit(Seq * x)          =0;
  virtual void visit(Call * x)         =0;
  virtual void visit(Idx * x)          =0;
  virtual void visit(SimpleAssign * x) =0;
  virtual void visit(IdxAssign * x)    =0;
  virtual void visit(IfElse * x)       =0;
};


class Parser {
  File file;
  std::vector<Token> tokens;
  std::vector<string*> strings;
  std::vector<double> doubles;
  std::unordered_map<string*,string*> symbols;
  int cursor;
  Seq* parseSequence();
  Exp* parseExp();
  Exp* parseBinExp(Exp* e);
  Exp* parsePrimaryExp();

public:
  Parser(File f): file(f) {
    Token tok;
    int count = 0;
    cursor = 0;
    while((tok = file.next()) != END) {
      count++;
      tokens.push_back(tok);
      string* str = NULL;
      if (tok == IDENT || tok == STR) {	
	if (symbols.count(file.ident_or_string) == 0) {
	  str = file.ident_or_string;
	  symbols.insert(std::make_pair(str, str));
	} else
	    str = symbols.at(file.ident_or_string);
      }
      strings.push_back( str );
      doubles.push_back( (tok == NUM) ? file.number : 0.0);
    }
    tokens.push_back(END);
  }

  Token token();
  string* string_value();
  double  double_value();
  void advance();
  Seq * parse();
};

} // namespace rift
#endif

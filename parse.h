#ifndef H_PARSE
#define H_PARSE

#include "llvm/IR/Verifier.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include <iostream>
#include <unordered_map>

#include "lex.h"

using std::endl;
using std::string;

class Exp {
public:
  virtual ~Exp(){}
  virtual void print(std::ostream& os) = 0;
  virtual llvm::Value *codegen() { return nullptr; } // FIXME
  friend std::ostream& operator<<(std::ostream& os, Exp& e);
};

std::ostream& operator<<(std::ostream& os, Exp& e);

class Num : public Exp {
  double value;
public:
  Num(double v) : value(v) {}
  void print(std::ostream& os) { os << value; }
  virtual llvm::Value *codegen() {
    return llvm::ConstantFP::get(llvm::getGlobalContext(), llvm::APFloat(value));
  }
};

class Str : public Exp {
  string* value;
public:
  Str(string* v) : value(v) {}
  void print(std::ostream& os) {
    os << "\"" << *value << "\"";
  }
};

class Var : public Exp {
  string* value;
public:
  Var(string* v) : value(v) {}
  void print(std::ostream& os) { os << *value; }
};

class Seq : public Exp {
  std::vector<Exp*> * exps;
public:
  Seq(std::vector<Exp*>* v) : exps(v) {}
  void print(std::ostream& os) {
    for(Exp* e : *exps) os << *e << endl;
  }
  ~Seq() {
    exps->clear();
    delete exps;
  }
};


class Fun : public Exp {
  std::vector<Var*> *params;
  Seq* body;
public:
  Fun(std::vector<Var*> *params, Seq *body) : params(params), body(body) {}
  void print(std::ostream& os) {
  os << "function (";
  for (Var* v : *params) os <<  *v << ",";
  os << ") {\n"; body->print(os); os << "}\n";
  }
  ~Fun() {
    params->clear();
    delete params;
    delete body;
  }
};

class BinExp : public Exp {
  Exp *left, *right;
  Token op;
public:
  BinExp(Exp* l, Token op, Exp* r) : left(l), op(op), right(r) {}
  void print(std::ostream& os) {
    left->print(os);  os << " " <<tok_to_str(op) << " ";
    right->print(os);
  }
  ~BinExp() {
    delete left; 
    delete right;
  }
};

class Call : public Exp {
  Var* name;
  std::vector<Exp*>* args;
public:
  Call(Var* n, std::vector<Exp*>* a) : name(n), args(a) {}
  void print(std::ostream& os) {  
    os << *name << "(";
    for (Exp* v : *args)  os <<  *v << ",";
    os <<")";
  }
  ~Call() {
    delete name;
    delete args;
  }
};

class Idx : public Exp {
  Var *name;
  Exp *body;
public:
  Idx(Var* n, Exp* b) : name(n), body(b) {}
  void print(std::ostream& os) {
    os << *name << "[";  body->print(os);  os << "]";
  }
  ~Idx() {
    delete name;
    delete body;
  }
};


class Assign : public Exp { };

class SimpleAssign : public Assign {
  Var* name;
  Exp* rhs;
public:
  SimpleAssign(Var* n, Exp* r) : name(n), rhs(r) {}
  void print(std::ostream& os) {
    name->print(os); os << " <- " ; rhs->print(os);
  }
  ~SimpleAssign() {
    delete name;
    delete rhs;
  }
};

class IdxAssign : public Assign {
  Idx *lhs;
  Exp *rhs;
public:
  IdxAssign(Idx* l, Exp* r) : lhs(l), rhs(r) {}
  void print(std::ostream& os) {
    lhs->print(os); os << " <- " ; rhs->print(os);
  }
  ~IdxAssign() {
    delete lhs;
    delete rhs;
  }
};

class IfElse : public Exp {
  Exp *guard;
  Seq *ifclause, *elseclause;
 
public:
  IfElse(Exp* g, Seq* i, Seq* e) : guard(g), ifclause(i), elseclause(e) {}
  void print(std::ostream& os) {
    os << "if ( "; guard->print(os);  os << " ) {\n";  
    ifclause->print(os);  os << "\n} else {\n";  
    elseclause->print(os);   os << "\n}";
  }
  ~IfElse() {
    delete guard;
    delete ifclause;
    delete elseclause;
  }
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
  Exp* parse();
};


#endif

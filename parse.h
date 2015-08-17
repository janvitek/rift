#include "llvm/IR/Verifier.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include <iostream>
#include <unordered_map>

#include "lex.h"

using std::cout;
using std::endl;
using std::string;

using namespace std;
using namespace llvm;

class Exp {
public:
  virtual ~Exp(){}
  virtual void print(ostream& os) = 0;
  virtual Value *codegen() { return 0; } 
  friend ostream& operator<<(ostream& os, const Exp& e);
};

ostream& operator<<(ostream& os, Exp& e);

class Num : public Exp {
  float value;
public:
  Num(float v) : value(v) {}
  void print(ostream& os) { os << value; }
  virtual Value *codegen() {
    return ConstantFP::get(getGlobalContext(), APFloat(value));
  }
};

class Str : public Exp {
  string* value;
public:
  Str(string* v) : value(v) {}
  void print(ostream& os) {
    os << "\"" << *value << "\"";
  }
};

class Var : public Exp {
  string* value;
public:
  Var(string* v) : value(v) {}
  void print(ostream& os) { os << *value; }
};

class Seq : public Exp {
  vector<Exp*> * exps;
public:
  Seq(vector<Exp*>* v) : exps(v) {}
  void print(ostream& os) {
    for(Exp* e : *exps) os << *e << endl;
  }
  ~Seq() {
    exps->clear();
    delete exps;
  }
};


class Fun : public Exp {
  vector<Var*> *params;
  Seq* body;
public:
  Fun(vector<Var*> *params, Seq *body) : params(params), body(body) {}
  void print(ostream& os) {
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
  void print(ostream& os) {
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
  vector<Exp*>* args;
public:
  Call(Var* n, vector<Exp*>* a) : name(n), args(a) {}
  void print(ostream& os) {  
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
  void print(ostream& os) {
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
  void print(ostream& os) {
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
  void print(ostream& os) {
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
  void print(ostream& os) {
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
  vector<Token> tokens;
  vector<string*> strings;
  vector<float> floats;
  unordered_map<string*,string*> symbols;
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
      floats.push_back( (tok == NUM) ? file.number : 0.0);
    }
    tokens.push_back(END);
  }

  Token token();
  string* string_value();
  float  float_value();
  void advance();
  Exp* parse();
};

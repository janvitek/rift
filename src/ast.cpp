#include "ast.h"
#include "pool.h"

namespace rift {
namespace ast {
/* A Str object holds an index in the constant pool,
   this returns the corresponding string. */
string const & Str::value() const {
    return Pool::getPoolObject(index);
}

/* A Var object holds an index in the constant pool,
   this returns the corresponding string. */
string const & Var::value() const {
    return Pool::getPoolObject(symbol);
}

// This is a simple visitor pattern implementtion...
void Num::accept(Visitor * v)              { v->visit(this); }
void Str::accept(Visitor * v)              { v->visit(this); }
void Var::accept(Visitor * v)              { v->visit(this); }
void Seq::accept(Visitor * v)              { v->visit(this); }
void Fun::accept(Visitor * v)              { v->visit(this); }
void BinExp::accept(Visitor * v)           { v->visit(this); }
void Call::accept(Visitor * v)             { v->visit(this); }
void UserCall::accept(Visitor * v)         { v->visit(this); }
void SpecialCall::accept(Visitor * v)      { v->visit(this); }
void CCall::accept(Visitor * v)            { v->visit(this); }
void EvalCall::accept(Visitor * v)         { v->visit(this); }
void TypeCall::accept(Visitor * v)         { v->visit(this); }
void LengthCall::accept(Visitor * v)       { v->visit(this); }
void Index::accept(Visitor * v)            { v->visit(this); }
void Assignment::accept(Visitor * v)       { v->visit(this); }
void SimpleAssignment::accept(Visitor * v) { v->visit(this); }
void IndexAssignment::accept(Visitor * v)  { v->visit(this); }
void IfElse::accept(Visitor * v)           { v->visit(this); }
void WhileLoop::accept(Visitor * v)        { v->visit(this); }
}
}

#include "parse.h"

using namespace std;

using namespace llvm;

using std::cout;


static Module *theModule;
static IRBuilder<> builder(getGlobalContext());
static std::map<std::string, Value*> namedValues;

ostream& operator<<(ostream& os, Exp& e) {
  e.print(os);
  return os;
}

Token Parser::token() {
  return tokens.at(cursor);
}

void Parser::advance() {
  cursor++;
}

string* Parser::string_value() {
  return strings.at(cursor);
}

double Parser::double_value() {
  return doubles.at(cursor);
}

#define EAT(T) \
  if (token() != T ) { error("Failed expecting ", tok_to_str(T)); } \
  else { advance(); }

#define EAT2(T, T2)       EAT(T); EAT(T2);
#define EAT3(T, T2, T3)	  EAT(T); EAT(T2); EAT(T3)

Exp* Parser::parseExp() {
  Exp *e = parseBinExp( parsePrimaryExp() );
  return e;
}

Seq* Parser::parseSequence() {
  vector<Exp*> *v = new vector<Exp*>();
  Exp *e = parseExp();
  v->push_back( e );
  if ( token() != SEP ) return new Seq ( v );
  while ( token() == SEP ) {
    while ( token() == SEP ) { EAT(SEP); }
    if ( token() == END ) break;
    v->push_back( parseExp() );
  }
  return new Seq( v );
}

Exp* Parser::parsePrimaryExp(){
  Exp * e = nullptr;
  Token t = token();
  if ( t == STR ) { 
    e = new Str(string_value());
    EAT(STR);
  } else if ( t == NUM ) { 
    e = new Num(double_value());
    EAT(NUM);
  } else if ( t == FUN ) {
    EAT2( FUN, OPAR);
    vector<Var*>* a = new vector<Var*>();
    while( token() != CPAR) {
      Var *v = new Var(string_value());
      EAT(IDENT);
      a->push_back(v);
      if ( token() == COM ) advance();
      else break;
    }
    EAT2( CPAR, OCBR );
    Seq* b = parseSequence();
    EAT(CCBR);
    e = new Fun(a, b);
  } else if ( t == IF ) {
    EAT2( IF, OPAR );
    Exp* g = parseExp();
    EAT2( CPAR, OCBR);
    Seq* t = parseSequence();
    EAT3( CCBR, ELSE, OCBR);
    Seq* f = parseSequence();
    EAT(CCBR);
    e = new IfElse(g,t,f);
  } else if ( t == IDENT) { 
    Var * v = new Var(string_value());
    EAT(IDENT);
    if ( token() == OPAR )  {
      EAT(OPAR);
      vector<Exp*>* a = new vector<Exp*>();
      while( token() != CPAR) {
	Exp *b = parseExp();
	a->push_back(b);
	if ( token() == COM ) { EAT(COM);}
	else break;
      }
      EAT(CPAR);
      e = new Call(v,a);
    } else if ( token() == OSBR ) {
      EAT(OSBR);
      Exp *i = parseExp();
      EAT(CSBR);
      Idx *idx = new Idx(v,i);
      if ( token() == ASS ) {
	EAT(ASS);
	Exp *b = parseExp();
	e = new IdxAssign(idx,b);
      } else 
	e = idx;
    } else if ( token() == ASS ) {
      EAT(ASS);
      Exp* b = parseExp();
      e = new SimpleAssign(v,b);
    } else 
      e = v;    
  }
  if ( e == nullptr ) error("Failed to parse Primary");
  return e;
}

Exp* Parser::parseBinExp(Exp* l) {
  Token t = token();
  if(t != DIV && t != PLUS && t != MIN && 
     t != MUL && t != LT   && t != EQ)    return l;

  advance();
  Exp * r = parsePrimaryExp();
  return parseBinExp(new BinExp(l, t, r));    
}

Exp* Parser::parse() {
  Exp* e = parseSequence();
  return e;
}


namespace {

Type * t_Int;
PointerType * p_IntVector;

Function * f_createIntVector;
Function * f_deleteIntVector;
Function * f_setIntVectorElement;
Function * f_getIntVectorElement;
Function * f_getIntVectorSize;

Module * initializeModule(std::string const & name) {
    Module * m = new Module(name, getGlobalContext());
    // now we must create the type declarations required in the runtime functions
    t_Int = IntegerType::get(getGlobalContext(), 32);
    StructType * t_IntVector = StructType::create(getGlobalContext(), "IntVector");
    std::vector<Type *> fields;
    fields.push_back(t_Int); // length
    fields.push_back(PointerType::get(t_Int, 0));
    t_IntVector->setBody(fields, false);
    p_IntVector = PointerType::get(t_IntVector, 0);
    // create the function types from the API
    fields = { t_Int };
    FunctionType * intVec_Int = FunctionType::get(p_IntVector, fields, false);
    fields = { p_IntVector };
    FunctionType * void_intVec = FunctionType::get(Type::getVoidTy(getGlobalContext()), fields, false);
    FunctionType * int_intVec = FunctionType::get(t_Int, fields, false);
    fields = { p_IntVector, t_Int };
    FunctionType * int_intVecInt = FunctionType::get(t_Int, fields, false);
    fields = { p_IntVector, t_Int, t_Int };
    FunctionType * void_intVecIntInt = FunctionType::get(Type::getVoidTy(getGlobalContext()), fields, false);
    // and finally create the function declarations
    f_createIntVector = Function::Create(intVec_Int, Function::ExternalLinkage, "createIntVector", m);
    f_deleteIntVector = Function::Create(void_intVec, Function::ExternalLinkage, "deleteIntVector", m);
    f_setIntVectorElement = Function::Create(void_intVecIntInt, Function::ExternalLinkage, "setIntVectorElement", m);
    f_getIntVectorElement = Function::Create(int_intVecInt, Function::ExternalLinkage, "getIntVectorElement", m);
    f_getIntVectorSize = Function::Create(int_intVec, Function::ExternalLinkage, "getIntVectorSize", m);
    // done
    return m;
}

} // namespace


int main(int n, char** argv) {
    // initialize the JIT
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();
    // create the module
    theModule = initializeModule("rift");



  File file(argv[1]);
  Parser parse(file);
  Exp* e = parse.parse();
  cout << *e << endl;
}

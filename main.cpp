#include "parse.h"

using namespace std;

using namespace llvm;

using std::cout;

extern  Module *theModule;
extern  IRBuilder<> builder;
extern  std::map<std::string, Value*> namedValues;


namespace {

Type * t_Int;
PointerType * p_IV;

Function * f_createIV;
Function * f_deleteIV;
Function * f_setIVElem;
Function * f_getIVElem;
Function * f_getIVSize;

Module * initializeModule(std::string const & name) {
    Module * m = new Module(name, getGlobalContext());
    // now we must create the type declarations required in the runtime functions
    t_Int = IntegerType::get(getGlobalContext(), 32);
    StructType * t_IV = StructType::create(getGlobalContext(), "IV");
    std::vector<Type *> fields;
    fields.push_back(t_Int); // length
    fields.push_back(PointerType::get(t_Int, 0));
    t_IV->setBody(fields, false);
    p_IV = PointerType::get(t_IV, 0);
    // create the function types from the API
    fields = { t_Int };
    FunctionType * intVec_Int = FunctionType::get(p_IV, fields, false);
    fields = { p_IV };
    FunctionType * void_intVec = FunctionType::get(Type::getVoidTy(getGlobalContext()), fields, false);
    FunctionType * int_intVec = FunctionType::get(t_Int, fields, false);
    fields = { p_IV, t_Int };
    FunctionType * int_intVecInt = FunctionType::get(t_Int, fields, false);
    fields = { p_IV, t_Int, t_Int };
    FunctionType * void_intVecIntInt = FunctionType::get(Type::getVoidTy(getGlobalContext()), fields, false);
    // and finally create the function declarations
    f_createIV = Function::Create(intVec_Int, Function::ExternalLinkage, "createIV", m);
    f_deleteIV = Function::Create(void_intVec, Function::ExternalLinkage, "deleteIV", m);
    f_setIVElem = Function::Create(void_intVecIntInt, Function::ExternalLinkage, "setIVElem", m);
    f_getIVElem = Function::Create(int_intVecInt, Function::ExternalLinkage, "getIVElem", m);
    f_getIVSize = Function::Create(int_intVec, Function::ExternalLinkage, "getIVSize", m);
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

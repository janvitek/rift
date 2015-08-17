#include "parse.h"

using namespace std;

using namespace llvm;

using std::cout;

extern  Module *theModule;
extern  IRBuilder<> builder;
extern  std::map<std::string, Value*> namedValues;


namespace {

Type * t_Int;
Type * t_Double;
PointerType * p_DV;

Function * f_createDV;
Function * f_concatDV;
Function * f_deleteDV;
Function * f_setDVElem;
Function * f_getDVElem;
Function * f_getDVSize;

Module * initializeModule(std::string const & name) {
    Module * m = new Module(name, getGlobalContext());
    // now we must create the type declarations required in the runtime functions
    t_Int = IntegerType::get(getGlobalContext(), 32);
    t_Double = Type::getDoubleTy(getGlobalContext());
    StructType * t_DV = StructType::create(getGlobalContext(), "DV");
    std::vector<Type *> fields;
    fields.push_back(t_Int); // length
    fields.push_back(PointerType::get(t_Double, 0));
    t_DV->setBody(fields, false);
    p_DV = PointerType::get(t_DV, 0);
    // create the function types from the API
    fields = { t_Double };
    FunctionType * dv_double = FunctionType::get(p_DV, fields, false);
    fields = { t_Int };
    FunctionType * dv_intvargs = FunctionType::get(p_DV, fields, true);
    fields = { p_DV };
    FunctionType * void_dv = FunctionType::get(Type::getVoidTy(getGlobalContext()), fields, false);
    FunctionType * double_dv = FunctionType::get(t_Double, fields, false);
    fields = { p_DV, t_Double };
    FunctionType * double_dvdouble = FunctionType::get(t_Double, fields, false);
    fields = { p_DV, t_Double, t_Double };
    FunctionType * void_dvdoubledouble = FunctionType::get(Type::getVoidTy(getGlobalContext()), fields, false);
    // and finally create the function declarations
    f_createDV = Function::Create(dv_double, Function::ExternalLinkage, "createDV", m);
    f_concatDV = Function::Create(dv_intvargs, Function::ExternalLinkage, "concatDV", m);
    f_deleteDV = Function::Create(void_dv, Function::ExternalLinkage, "deleteDV", m);
    f_setDVElem = Function::Create(void_dvdoubledouble, Function::ExternalLinkage, "setDVElem", m);
    f_getDVElem = Function::Create(double_dvdouble, Function::ExternalLinkage, "getDVElem", m);
    f_getDVSize = Function::Create(double_dv, Function::ExternalLinkage, "getDVSize", m);
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

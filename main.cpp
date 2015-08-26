#include "llvm/IR/Verifier.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "parse.h"
#include "runtime.h"


using namespace std;
using namespace llvm;


FunPtr test_compileFunction(rift::Fun * f);


int main(int n, char** argv) {
  // initialize the JIT
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeAsmParser();

  File file(argv[1]);
  rift::Parser parse(file);
  rift::Seq* e = parse.parse();
  // now we need the compiler
  rift::Fun * f = new rift::Fun();
  f->setBody(e);
  FunPtr x = reinterpret_cast<FunPtr>(test_compileFunction(f));
  // create the global environment
  Env * globalEnv = r_env_mk(nullptr, 0);

  RVal * result = x(globalEnv);
  //e->print();
  result->print();

  delete globalEnv;
  cout << endl;

}

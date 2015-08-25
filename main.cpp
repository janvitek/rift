#include "llvm/IR/Verifier.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "parse.h"
#include "runtime.h"


using namespace std;
using namespace llvm;

using std::cout;

int main(int n, char** argv) {
  // initialize the JIT
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeAsmParser();

  File file(argv[1]);
  rift::Parser parse(file);
  rift::Exp* e = parse.parse();
  // now we need the compiler


  // create the global environment
  Env * globalEnv = r_env_mk(nullptr, 0);

  e->print();

  delete globalEnv;
  cout << endl;

}

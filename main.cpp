#include "parse.h"

#include "llvm/IR/Verifier.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

using namespace std;
using namespace llvm;

using std::cout;

int main(int n, char** argv) {
  // initialize the JIT
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeAsmParser();

  File file(argv[1]);
  Parser parse(file);
  Exp* e = parse.parse();
  e->print();
  cout<<endl;
}

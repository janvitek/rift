#include "parse.h"

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
  cout << *e << endl;
}

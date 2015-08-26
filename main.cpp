#include <iostream>
#include <fstream>


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


RVal * eval_internal(Env * env, int size, char * source) {
    File file(size, source);
    rift::Parser parse(file);
    rift::Seq* e = parse.parse();
    // now we need the compiler
    rift::Fun * f = new rift::Fun();
    f->setBody(e);
    FunPtr x = reinterpret_cast<FunPtr>(test_compileFunction(f));
    // create the global environment
    RVal * result = x(env);
    return result;
}


int main(int argc, char** argv) {
  // initialize the JIT
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeAsmParser();
  // load the file
  assert (argc == 2 and "First argument must be file to run");
  ifstream fin(argv[1]);
  assert (fin.good() and "IO Error");
  fin.seekg(0, ios::end);
  int size = fin.tellg();
  char * buffer = new char[size];
  fin.seekg(0);
  fin.read(buffer, size);
  fin.close();
  Env * env = r_env_mk(nullptr, 0);
  RVal * result = eval_internal(env, size, buffer);
  delete env;
  result->print();

  cout << endl;

}

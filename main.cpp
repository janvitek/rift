#include <iostream>
#include <fstream>


#include "llvm/IR/Verifier.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "ast.h"
#include "parser.h"
#include "runtime.h"
#include "compiler.h"


using namespace std;
using namespace llvm;
using namespace rift;


void interactive() {
    cout << "rift console - type exit to quit" << endl;
    Environment * env = new Environment(nullptr);
    while (true) {
        try {
            cout << "> ";
            std::string in;
            getline(cin, in);
            if (in == "exit")
                break;
            eval(env, in.c_str())->print(cout);
            cout << endl;
        } catch (char const * error) {
            std::cerr << error << std::endl;
            std::cout << std::endl;
        }
    }
    delete env;
}

void runScript(char const * filename) {
    std::ifstream s(filename);
    if (not s.is_open()) {
        std::cerr << "Unable to open file " << filename << endl;
    } else {
        Parser p;
        ast::Fun * x = new ast::Fun(p.parse(s));
        Environment * env = new Environment(nullptr);
        compile(x)(env)->print(cout);
        delete x;
        delete env;
    }

}


int main(int argc, char * argv[]) {
    // initialize the JIT
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();
    if (argc == 1) {
        interactive();
    } else {
        if (argc > 2)
            cerr << "Only one script can be loaded at a time" << endl;
        else
            runScript(argv[1]);
    }
}




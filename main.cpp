#include <iostream>
#include <fstream>

#include "llvm.h"
#include "ast.h"
#include "parser.h"
#include "runtime.h"
#include "compiler.h"
#include "tests.h"

using namespace std;
using namespace llvm;
using namespace rift;

void interactive() {
    cout << "rift console - type exit to quit" << endl;
    Environment * env = new Environment(nullptr);
    while (not cin.eof()) {
        try {
            cout << "> ";
            std::string in;
            while (true) {
                std::string i;
                getline(cin, i);
                if (i.empty()) break;
                if (i.at(i.length() - 1) == '\\') {
                    in.append(i.substr(0, i.length() - 1));
                    in.append("\n");
                } else {
                    in.append(i);
                    break;
                }
            }
            if (in == "exit")
                break;
            if (in.empty())
                continue;
            in = in + "\n";
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
        tests();
        interactive();
    } else {
        if (argc > 2)
            cerr << "Only one script can be loaded at a time" << endl;
        else
            runScript(argv[1]);
    }
}




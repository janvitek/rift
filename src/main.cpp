#include <iostream>
#include <fstream>
#include <chrono>

#include "llvm.h"
#include "ast.h"
#include "parser.h"
#include "runtime.h"
#include "tests.h"
#include "rift.h"
#include "gc.h"

#include "compiler/jit.h"

using namespace std;
using namespace llvm;
using namespace rift;

extern double eval_time;

bool DEBUG = false;

void interactive() {
    cout << "rift console - type exit to quit" << endl;
    Environment * env = Environment::New(nullptr);
    while (not cin.eof()) {
        try {
            cout << "> ";
            string in;
            while (true) {
                string i;
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
            auto start = chrono::high_resolution_clock::now();
            RVal * x = eval(env, in.c_str());
            auto t = chrono::high_resolution_clock::now() - start;
            cout << *x << endl;
            double total_t = static_cast<double>(t.count()) / chrono::high_resolution_clock::period::den;
            cout << "Evaluation:               " << eval_time << "[s]" << endl;
            cout << "Evaluation & compilation: " << total_t << endl;
            cout << "Compilation:              " << (1 - eval_time/total_t)*100 << "[%]" << endl;
        } catch (char const * error) {
            cerr << error << endl;
            cout << endl;
        }
    }
}

void runScript(char const * filename) {
    ifstream s(filename);
    if (not s.is_open()) {
        cerr << "Unable to open file " << filename << endl;
    } else {
        Parser p;
        ast::Fun * x = new ast::Fun(p.parse(s));
        Environment * env = Environment::New(nullptr);
        auto res = JIT::compile(x)(env);
        res->print(cout);
    }

}

int main(int argc, char * argv[]) {
    // initialize the JIT
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();

    // force GC to be initialized:
    Environment::New(nullptr);

    int argPos = 1;
    if (argc > argPos) {
        if (0 == strncmp("-d", argv[argPos], 2)) {
            DEBUG = true;
            argPos++;
        }
    }
    if (argc == argPos) {
        tests();
        interactive();
    } else {
        if (argc > argPos+1) {
            cerr << "Only one script can be loaded at a time" << endl;
        } else {
            try {
                runScript(argv[argPos]);
            } catch (char const * error) {
                cerr << error << endl;
                cout << endl;
                exit(1);
            }
        }
    }
}




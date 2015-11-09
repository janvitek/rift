#include <iostream>
#include <fstream>
#include <sstream>

#include "llvm.h"
#include "ast.h"
#include "parser.h"

using namespace std;
using namespace llvm;
using namespace rift;

void interactive() {
    cout << "rift console - type exit to quit" << endl;
    while (true) {
        try {
            cout << "> ";
            std::string in;
            getline(cin, in);
            if (in == "exit")
                break;
            if (in.empty())
                continue;
            Parser p;
            auto exp = p.parse(in);
            exp->print(std::cout);
            delete exp;
        } catch (char const * error) {
            std::cerr << error << std::endl;
            std::cout << std::endl;
        }
    }
}

void test();

int main(int argc, char * argv[]) {
    test();
    interactive();
}

void test(std::string in, std::string expected) {
    Parser p;
    stringstream res;
    p.parse(in)->print(res);
    if (res.str().compare(expected) != 0) {
        std::cout << "Expected '" << in << "' to be printed as '"
                  << expected << "' but got '" << res.str() << "'\n";
        exit(1);
    }
}

void test() {
    test("3", "3\n");
    test(" 3 ", "3\n");
    test("function(){1}", "function() {\n1\n}\n");
}



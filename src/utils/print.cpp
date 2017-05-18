#include <iostream>
#include "printer.h"


namespace rift {

    void Printer::print(ast::Exp * exp, std::ostream & s) {
        Printer p(s);
        exp->accept(&p);
    }

    void Printer::print(ast::Exp * exp) {
        return print(exp, std::cout);
    }

}


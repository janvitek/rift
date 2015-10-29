#ifndef COMPILER_H
#define COMPILER_H

#include "ast.h"

namespace rift {

FunPtr compile(rift::ast::Fun * f);

}

#endif

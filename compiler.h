#ifndef COMPILER_H
#define COMPILER_H

#include <ciso646>

#include "ast.h"

namespace rift {

FunPtr compile(rift::ast::Fun * f);

}

#endif

#include <cassert>
#include <cstdarg>
#include <cstring>
#include <vector>
#include <iostream>
#include <unordered_map>

#include "runtime.h"
#include "lexer.h"
#include "parser.h"
#include "compiler.h"


using namespace std;
using namespace rift;


extern "C" {

Environment * envCreate(Environment * parent) {
    return new Environment(parent);
}

Value * envGet(Environment * env, int symbol) {
    return env->get(symbol);
}

Value * envSet(Environment * env, int symbol, Value * value) {
    env->set(symbol, value);
    return value;
}

DoubleVector * doubleVectorLiteral(double value) {
    return new DoubleVector(value);
}

CharacterVector * characterVectorLiteral(int cpIndex) {
    std::string const & original = Runtime::getPoolObject(cpIndex);
    char * dest = new char[original.size() + 1];
    memcpy(dest, original.c_str(), original.size());
    dest[original.size()] = 0;
    return new CharacterVector(dest, original.size());

}

Value * fromDoubleVector(DoubleVector * from) {
    return new Value(from);
}

Value * fromCharacterVector(CharacterVector * from) {
    return new Value(from);
}

Value * fromFunction(Function * from) {
    return new Value(from);
}


DoubleVector * doubleGetElement(DoubleVector * from, DoubleVector * index) {
    unsigned resultSize = index->size;
    double * result = new double[resultSize];
    for (unsigned i = 0; i < resultSize; ++i) {
        unsigned idx = index->data[i];
        if (idx < 0 or idx >= from->size)
            throw "Index out of bound";
        result[i] = from->data[idx];
    }
    return new DoubleVector(result, resultSize);
}

CharacterVector * characterGetElement(CharacterVector * from, DoubleVector * index) {
    unsigned resultSize = index->size;
    char * result = new char[resultSize];
    for (unsigned i = 0; i < resultSize; ++i) {
        unsigned idx = index->data[i];
        if (idx < 0 or idx >= from->size)
            throw "Index out of bound";
        result[i] = from->data[idx];
    }
    return new CharacterVector(result, resultSize);
}


Value * genericGetElement(Value * from, Value * index) {
    if (index->type != Value::Type::Double)
        throw "Index vector must be double";
    switch (from->type) {
    case Value::Type::Double:
        return new Value(doubleGetElement(from->d, index->d));
    case Value::Type::Character:
        return new Value(characterGetElement(from->c, index->d));
    default:
        throw "Cannot index a function";
    }
}

void doubleSetElement(DoubleVector * target, DoubleVector * index, DoubleVector * value) {
    for (unsigned i = 0; i < index->size; ++i) {
        unsigned idx = index->data[i];
        if (idx < 0 or idx >= target->size)
            throw "Index out of bound";
        double val = value->data[i % value->size];
        target->data[idx] = val;
    }
}

void characterSetElement(CharacterVector * target, DoubleVector * index, CharacterVector * value) {
    for (unsigned i = 0; i < index->size; ++i) {
        unsigned idx = index->data[i];
        if (idx < 0 or idx >= target->size)
            throw "Index out of bound";
        char val = value->data[i % value->size];
        target->data[idx] = val;
    }
}

void genericSetElement(Value * target, Value * index, Value * value) {
    if (index->type != Value::Type::Double)
        throw "Index vector must be double";
    if (target->type != value->type)
        throw "Vector and element must be of same type";
    switch (target->type) {
    case Value::Type::Double:
        doubleSetElement(target->d, index->d, value->d);
        break;
    case Value::Type::Character:
        characterSetElement(target->c, index->d, value->c);
        break;
    default:
        throw "Cannot index a function";
    }

}

DoubleVector * doubleAdd(DoubleVector * lhs, DoubleVector * rhs) {
    int resultSize = max(lhs->size, rhs->size);
    double * result = new double[resultSize];
    for (int i = 0; i < resultSize; ++i)
        result[i] = lhs->data[i % lhs->size] + rhs->data[i % rhs->size];
    return new DoubleVector(result, resultSize);
}

CharacterVector * characterAdd(CharacterVector * lhs, CharacterVector * rhs) {
    int resultSize = lhs->size + rhs->size;
    char * result = new char(resultSize + 1);
    memcpy(result, lhs->data, lhs->size);
    memcpy(result + lhs->size, rhs->data, rhs->size);
    result[resultSize] = 0;
    return new CharacterVector(result, resultSize);
}


Value * genericAdd(Value * lhs, Value * rhs) {
    if (lhs->type != rhs->type)
        throw "Incompatible types for binary operator";
    switch (lhs->type) {
    case Value::Type::Double:
        return new Value(doubleAdd(lhs->d, rhs->d));
    case Value::Type::Character:
        return new Value(characterAdd(lhs->c, rhs->c));
    default:
        throw "Invalid types for binary add";
    }
}

DoubleVector * doubleSub(DoubleVector * lhs, DoubleVector * rhs) {
    int resultSize = max(lhs->size, rhs->size);
    double * result = new double[resultSize];
    for (int i = 0; i < resultSize; ++i)
        result[i] = lhs->data[i % lhs->size] - rhs->data[i % rhs->size];
    return new DoubleVector(result, resultSize);
}



Value * genericSub(Value * lhs, Value * rhs) {
    if (lhs->type != rhs->type)
        throw "Incompatible types for binary operator";
    switch (lhs->type) {
    case Value::Type::Double:
        return new Value(doubleSub(lhs->d, rhs->d));
    default:
        throw "Invalid types for binary sub";
    }
}

DoubleVector * doubleMul(DoubleVector * lhs, DoubleVector * rhs) {
    int resultSize = max(lhs->size, rhs->size);
    double * result = new double[resultSize];
    for (int i = 0; i < resultSize; ++i)
        result[i] = lhs->data[i % lhs->size] * rhs->data[i % rhs->size];
    return new DoubleVector(result, resultSize);
}

Value * genericMul(Value * lhs, Value * rhs) {
    if (lhs->type != rhs->type)
        throw "Incompatible types for binary operator";
    switch (lhs->type) {
    case Value::Type::Double:
        return new Value(doubleMul(lhs->d, rhs->d));
    default:
        throw "Invalid types for binary mul";
    }
}

DoubleVector * doubleDiv(DoubleVector * lhs, DoubleVector * rhs) {
    int resultSize = max(lhs->size, rhs->size);
    double * result = new double[resultSize];
    for (int i = 0; i < resultSize; ++i)
        result[i] = lhs->data[i % lhs->size] / rhs->data[i % rhs->size];
    return new DoubleVector(result, resultSize);
}

Value * genericDiv(Value * lhs, Value * rhs) {
    if (lhs->type != rhs->type)
        throw "Incompatible types for binary operator";
    switch (lhs->type) {
    case Value::Type::Double:
        return new Value(doubleDiv(lhs->d, rhs->d));
    default:
        throw "Invalid types for binary div";
    }
}

DoubleVector * doubleEq(DoubleVector * lhs, DoubleVector * rhs) {
    int resultSize = max(lhs->size, rhs->size);
    double * result = new double[resultSize];
    for (int i = 0; i < resultSize; ++i)
        result[i] = lhs->data[i % lhs->size] == rhs->data[i % rhs->size];
    return new DoubleVector(result, resultSize);
}

DoubleVector * characterEq(CharacterVector * lhs, CharacterVector * rhs) {
    int resultSize = max(lhs->size, rhs->size);
    double * result = new double[resultSize];
    for (int i = 0; i < resultSize; ++i)
        result[i] = lhs->data[i % lhs->size] == rhs->data[i % rhs->size];
    return new DoubleVector(result, resultSize);
}

DoubleVector * functionEq(Function * lhs, Function * rhs) {
    return new DoubleVector(lhs->code == rhs->code);
}

Value * genericEq(Value * lhs, Value * rhs) {
    if (lhs->type != rhs->type)
        return new Value(new DoubleVector(0));
    switch (lhs->type) {
    case Value::Type::Double:
        return new Value(doubleEq(lhs->d, rhs->d));
    case Value::Type::Character:
        return new Value(characterEq(lhs->c, rhs->c));
    case Value::Type::Function:
    default:
        return new Value(functionEq(lhs->f, rhs->f));
    }
}

DoubleVector * doubleNeq(DoubleVector * lhs, DoubleVector * rhs) {
    int resultSize = max(lhs->size, rhs->size);
    double * result = new double[resultSize];
    for (int i = 0; i < resultSize; ++i)
        result[i] = lhs->data[i % lhs->size] != rhs->data[i % rhs->size];
    return new DoubleVector(result, resultSize);
}

DoubleVector * characterNeq(CharacterVector * lhs, CharacterVector * rhs) {
    int resultSize = max(lhs->size, rhs->size);
    double * result = new double[resultSize];
    for (int i = 0; i < resultSize; ++i)
        result[i] = lhs->data[i % lhs->size] == rhs->data[i % rhs->size];
    return new DoubleVector(result, resultSize);
}

DoubleVector * functionNeq(Function * lhs, Function * rhs) {
    return new DoubleVector(lhs->code != rhs->code);
}

Value * genericNeq(Value * lhs, Value * rhs) {
    if (lhs->type != rhs->type)
        return new Value(new DoubleVector(1));
    switch (lhs->type) {
    case Value::Type::Double:
        return new Value(doubleNeq(lhs->d, rhs->d));
    case Value::Type::Character:
        return new Value(characterNeq(lhs->c, rhs->c));
    case Value::Type::Function:
    default:
        return new Value(functionNeq(lhs->f, rhs->f));
    }
}

DoubleVector * doubleLt(DoubleVector * lhs, DoubleVector * rhs) {
    int resultSize = max(lhs->size, rhs->size);
    double * result = new double[resultSize];
    for (int i = 0; i < resultSize; ++i)
        result[i] = lhs->data[i % lhs->size] < rhs->data[i % rhs->size];
    return new DoubleVector(result, resultSize);
}

Value * genericLt(Value * lhs, Value * rhs) {
    if (lhs->type != rhs->type)
        throw "Incompatible types for binary operator";
    switch (lhs->type) {
    case Value::Type::Double:
        return new Value(doubleLt(lhs->d, rhs->d));
    default:
        throw "Invalid types for lt";
    }
}

DoubleVector * doubleGt(DoubleVector * lhs, DoubleVector * rhs) {
    int resultSize = max(lhs->size, rhs->size);
    double * result = new double[resultSize];
    for (int i = 0; i < resultSize; ++i)
        result[i] = lhs->data[i % lhs->size] > rhs->data[i % rhs->size];
    return new DoubleVector(result, resultSize);
}


Value * genericGt(Value * lhs, Value * rhs) {
    if (lhs->type != rhs->type)
        throw "Incompatible types for binary operator";
    switch (lhs->type) {
    case Value::Type::Double:
        return new Value(doubleGt(lhs->d, rhs->d));
    default:
        throw "Invalid types for gt";
    }
}

Function * createFunction(int index, Environment * env) {
    return new Function(Runtime::getFunction(index), env);
}

bool toBoolean(Value * value) {
    switch (value->type) {
    case Value::Type::Function:
        return true;
    case Value::Type::Character:
        return (value->c->size > 0) and (value->c->data[0] != 0);
    case Value::Type::Double:
        return (value->d->size > 0) and (value->d->data[0] != 0);
    default:
        return false;
    }

}

Value * call(Value * callee, Environment * parent, unsigned argc, ...) {
    if (callee->type != Value::Type::Function)
        throw "Only functions can be called";
    Function * f = callee->f;
    if (f->argsSize != argc)
        throw "Invalid number of arguments given";
    Environment * calleeEnv = new Environment(parent);
    va_list ap;
    va_start(ap, argc);
    for (unsigned i = 0; i < argc; ++i)
        // TODO this is pass by reference always!
        calleeEnv->set(f->args[i], va_arg(ap, Value *));
    va_end(ap);
    return f->code(calleeEnv);
}

double length(Value * value) {
    switch (value->type) {
    case Value::Type::Double:
        return value->d->size;
    case Value::Type::Character:
        return value->c->size;
    default:
        throw "Cannot determine length of a function";
    }

}

CharacterVector * type(Value * value) {
    switch (value->type) {
    case Value::Type::Double:
        return CharacterVector::copy("double");
    case Value::Type::Character:
        return CharacterVector::copy("character");
    case Value::Type::Function:
        return CharacterVector::copy("function");
    default:
        return nullptr;
    }
}

Value * eval(Environment * env, char const * value) {
    std::string s(value);
    Parser p;
    ast::Fun * x = new ast::Fun(p.parse(s));
    FunPtr f = compile(x);
    Value * result = f(env);
    delete x;
    return result;
}

Value * characterEval(Environment * env, CharacterVector * value) {
    if (value->size == 0)
        throw "Cannot evaluate empty character vector";
    return eval(env, value->data);
}

Value * genericEval(Environment * env, Value * value) {
    if (value->type != Value::Type::Character)
        throw "Only character vectors can be evaluated";
    return characterEval(env, value->c);
}

Value * c(int size, ...) {
    if (size == 0)
        return new Value(new DoubleVector(nullptr, 0));
    std::vector<Value *> args;
    va_list ap;
    va_start(ap, size);
    for (int i = 0; i < size; ++i)
        args.push_back(va_arg(ap, Value*));
    va_end(ap);
    Value::Type t = args[0]->type;
    if (t == Value::Type::Function)
        throw "Cannot concatenate functions";
    for (unsigned i = 1; i < args.size(); ++i) {
        if (args[i]->type != t)
            throw "Types of all c arguments must be the same";
    }
    if (t == Value::Type::Double) {
        int size = 0;
        for (Value * v : args)
            size += v->d->size;
        double * result = new double[size];
        int offset = 0;
        for (Value * v : args) {
            memcpy(result + offset, v->d->data, v->d->size * sizeof(double));
            offset += v->d->size;
        }
        return new Value(new DoubleVector(result, size));
    } else { // Character
        int size = 0;
        for (Value * v : args)
            size += v->c->size;
        char * result = new char[size];
        int offset = 0;
        for (Value * v : args) {
            memcpy(result + offset, v->d->data, v->c->size * sizeof(char));
            offset += v->c->size;
        }
        return new Value(new CharacterVector(result, size));
    }
}





} // extern "C"

namespace rift {
std::vector<Function *> Runtime::f_;
std::vector<std::string> Runtime::pool_;

}


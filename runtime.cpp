
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
#include "pool.h"

using namespace std;
using namespace rift;


extern "C" {

Environment * envCreate(Environment * parent) {
    return new Environment(parent);
}

RVal * envGet(Environment * env, int symbol) {
    return env->get(symbol);
}

void envSet(Environment * env, int symbol, RVal * RVal) {
    env->set(symbol, RVal);
}

DoubleVector * doubleVectorLiteral(double RVal) {
    return new DoubleVector(RVal);
}

CharacterVector * characterVectorLiteral(int cpIndex) {
    std::string const & original = Pool::getPoolObject(cpIndex);
    return new CharacterVector(original.c_str());

}

RVal * fromDoubleVector(DoubleVector * from) {
    return new RVal(from);
}

RVal * fromCharacterVector(CharacterVector * from) {
    return new RVal(from);
}

RVal * fromFunction(RFun * from) {
    return new RVal(from);
}

DoubleVector * doubleFromValue(RVal * v) {
    if (v->type != RVal::Type::Double)
        throw "Invalid type";
    return v->d;
}

double scalarFromVector(DoubleVector * v) {
    if (v->size != 1)
        throw "not a scalar";
    return v->data[0];
}

CharacterVector * characterFromValue(RVal *v) {
    if (v->type != RVal::Type::Character)
        throw "Invalid type";
    return v->c;

}
RFun * functionFromValue(RVal *v) {
    if (v->type != RVal::Type::Function)
        throw "Invalid type";
    return v->f;

}


double doubleGetSingleElement(DoubleVector * from, double index) {
    if (index < 0 or index >= from->size)
        throw "Index out of bounds";
    return from->data[static_cast<int>(index)];
}

DoubleVector * doubleGetElement(DoubleVector * from, DoubleVector * index) {
    unsigned resultSize = index->size;
    double * result = new double[resultSize];
    for (unsigned i = 0; i < resultSize; ++i) {
        int idx = static_cast<int>(index->data[i]);
        if ((idx < 0) or static_cast<unsigned>(idx) >= from->size)
            throw "Index out of bounds";
        result[i] = from->data[idx];
    }
    return new DoubleVector(result, resultSize);
}

CharacterVector * characterGetElement(CharacterVector * from, DoubleVector * index) {
    unsigned resultSize = index->size;
    CharacterVector * result = new CharacterVector(resultSize);
    for (unsigned i = 0; i < resultSize; ++i) {
        int idx = static_cast<int>(index->data[i]);
        if (idx < 0 or static_cast<unsigned>(idx) >= from->size)
            throw "Index out of bounds";
        result->data[i] = from->data[idx];
    }
    return result;
}


RVal * genericGetElement(RVal * from, RVal * index) {
    if (index->type != RVal::Type::Double)
        throw "Index vector must be double";
    switch (from->type) {
    case RVal::Type::Double:
        return new RVal(doubleGetElement(from->d, index->d));
    case RVal::Type::Character:
        return new RVal(characterGetElement(from->c, index->d));
    default:
        throw "Cannot index a function";
    }
}

void doubleSetElement(DoubleVector * target, DoubleVector * index, DoubleVector * RVal) {
    for (unsigned i = 0; i < index->size; ++i) {
        int idx = static_cast<int>(index->data[i]);
        if (idx < 0 or static_cast<unsigned>(idx) >= target->size)
            throw "Index out of bound";
        double val = RVal->data[i % RVal->size];
        target->data[idx] = val;
    }
}

void scalarSetElement(DoubleVector * target, double index, double RVal) {
    int idx = static_cast<int>(index);
    if (idx < 0 or static_cast<unsigned>(idx) >= target->size)
        throw "Index out of bound";
    target->data[idx] = RVal;
}

void characterSetElement(CharacterVector * target, DoubleVector * index, CharacterVector * RVal) {
    for (unsigned i = 0; i < index->size; ++i) {
        int idx = static_cast<int>(index->data[i]);
        if (idx < 0 or static_cast<unsigned>(idx) >= target->size)
            throw "Index out of bound";
        char val = RVal->data[i % RVal->size];
        target->data[idx] = val;
    }
}

void genericSetElement(RVal * target, RVal * index, RVal * RVal) {
    if (index->type != RVal::Type::Double)
        throw "Index vector must be double";
    if (target->type != RVal->type)
        throw "Vector and element must be of same type";
    switch (target->type) {
    case RVal::Type::Double:
        doubleSetElement(target->d, index->d, RVal->d);
        break;
    case RVal::Type::Character:
        characterSetElement(target->c, index->d, RVal->c);
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
    CharacterVector * result = new CharacterVector(resultSize);
    memcpy(result->data, lhs->data, lhs->size);
    memcpy(result->data + lhs->size, rhs->data, rhs->size);
    return result;
}


RVal * genericAdd(RVal * lhs, RVal * rhs) {
    if (lhs->type != rhs->type)
        throw "Incompatible types for binary operator";
    switch (lhs->type) {
    case RVal::Type::Double:
        return new RVal(doubleAdd(lhs->d, rhs->d));
    case RVal::Type::Character:
        return new RVal(characterAdd(lhs->c, rhs->c));
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



RVal * genericSub(RVal * lhs, RVal * rhs) {
    if (lhs->type != rhs->type)
        throw "Incompatible types for binary operator";
    switch (lhs->type) {
    case RVal::Type::Double:
        return new RVal(doubleSub(lhs->d, rhs->d));
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

RVal * genericMul(RVal * lhs, RVal * rhs) {
    if (lhs->type != rhs->type)
        throw "Incompatible types for binary operator";
    switch (lhs->type) {
    case RVal::Type::Double:
        return new RVal(doubleMul(lhs->d, rhs->d));
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

RVal * genericDiv(RVal * lhs, RVal * rhs) {
    if (lhs->type != rhs->type)
        throw "Incompatible types for binary operator";
    switch (lhs->type) {
    case RVal::Type::Double:
        return new RVal(doubleDiv(lhs->d, rhs->d));
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

RVal * genericEq(RVal * lhs, RVal * rhs) {
    if (lhs->type != rhs->type)
        return new RVal(new DoubleVector(0));
    switch (lhs->type) {
    case RVal::Type::Double:
        return new RVal(doubleEq(lhs->d, rhs->d));
    case RVal::Type::Character:
        return new RVal(characterEq(lhs->c, rhs->c));
    case RVal::Type::Function:
    default:
        return new RVal(new DoubleVector(lhs->f->code == rhs->f->code));
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

RVal * genericNeq(RVal * lhs, RVal * rhs) {
    if (lhs->type != rhs->type)
        return new RVal(new DoubleVector(1));
    switch (lhs->type) {
    case RVal::Type::Double:
        return new RVal(doubleNeq(lhs->d, rhs->d));
    case RVal::Type::Character:
        return new RVal(characterNeq(lhs->c, rhs->c));
    case RVal::Type::Function:
    default:
        return new RVal(new DoubleVector(lhs->f->code != rhs->f->code));
    }
}

DoubleVector * doubleLt(DoubleVector * lhs, DoubleVector * rhs) {
    int resultSize = max(lhs->size, rhs->size);
    double * result = new double[resultSize];
    for (int i = 0; i < resultSize; ++i)
        result[i] = lhs->data[i % lhs->size] < rhs->data[i % rhs->size];
    return new DoubleVector(result, resultSize);
}

RVal * genericLt(RVal * lhs, RVal * rhs) {
    if (lhs->type != rhs->type)
        throw "Incompatible types for binary operator";
    switch (lhs->type) {
    case RVal::Type::Double:
        return new RVal(doubleLt(lhs->d, rhs->d));
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


RVal * genericGt(RVal * lhs, RVal * rhs) {
    if (lhs->type != rhs->type)
        throw "Incompatible types for binary operator";
    switch (lhs->type) {
    case RVal::Type::Double:
        return new RVal(doubleGt(lhs->d, rhs->d));
    default:
        throw "Invalid types for gt";
    }
}

RFun * createFunction(int index, Environment * env) {
    return new RFun(Pool::getFunction(index), env);
}

bool toBoolean(RVal * RVal) {
    switch (RVal->type) {
    case RVal::Type::Function:
        return true;
    case RVal::Type::Character:
        return (RVal->c->size > 0) and (RVal->c->data[0] != 0);
    case RVal::Type::Double:
        return (RVal->d->size > 0) and (RVal->d->data[0] != 0);
    default:
        return false;
    }

}

RVal * call(RVal * callee, unsigned argc, ...) {
    if (callee->type != RVal::Type::Function) throw "Not a function!";
    RFun * f = callee->f;
    if (f->argsSize != argc) throw "Wrong number of arguments";
    assert(false and "You have to implement me too!");
    return nullptr;
}

double length(RVal * RVal) {
    switch (RVal->type) {
        // TODO: implement length
    case RVal::Type::Function:
        throw "Cannot determine length of a function";
    default:
        throw "Cannot determine length of unknown object";
    }

}

CharacterVector * type(RVal * RVal) {
    switch (RVal->type) {
    case RVal::Type::Double:
        return new CharacterVector("double");
    case RVal::Type::Character:
        return new CharacterVector("character");
    case RVal::Type::Function:
        return new CharacterVector("function");
    default:
        return nullptr;
    }
}

RVal * eval(Environment * env, char const * value) {
    std::string s(value);
    Parser p;
    ast::Fun * x = new ast::Fun(p.parse(s));
    if (x->body->body.empty())
        return new RVal({0.0});
    FunPtr f = compile(x);
    RVal * result = f(env);
    delete x;
    return result;
}

RVal * characterEval(Environment * env, CharacterVector * RVal) {
    if (RVal->size == 0)
        throw "Cannot evaluate empty character vector";
    return eval(env, RVal->data);
}

RVal * genericEval(Environment * env, RVal * RVal) {
    if (RVal->type != RVal::Type::Character)
        throw "Only character vectors can be evaluated";
    return characterEval(env, RVal->c);
}

DoubleVector * doublec(int size, ...) {
    std::vector<DoubleVector *> args;
    va_list ap;
    va_start(ap, size);
    for (int i = 0; i < size; ++i)
        args.push_back(va_arg(ap, DoubleVector*));
    va_end(ap);
    size = 0;
    for (DoubleVector * v : args)
        size += v->size;
    double * result = new double[size];
    int offset = 0;
    for (DoubleVector * v : args) {
        memcpy(result + offset, v->data, v->size * sizeof(double));
        offset += v->size;
    }
    return new DoubleVector(result, size);
}

CharacterVector * characterc(int size, ...) {
    std::vector<CharacterVector *> args;
    va_list ap;
    va_start(ap, size);
    for (int i = 0; i < size; ++i)
        args.push_back(va_arg(ap, CharacterVector*));
    va_end(ap);
    size = 0;
    for (CharacterVector * v : args)
        size += v->size;
    CharacterVector * result = new CharacterVector(size);
    int offset = 0;
    for (CharacterVector * v : args) {
        memcpy(result->data + offset, v->data, v->size * sizeof(char));
        offset += v->size;
    }
    return result;
}

RVal * c(int size, ...) {
    if (size == 0)
        return new RVal(new DoubleVector(nullptr, 0));
    std::vector<RVal *> args;
    va_list ap;
    va_start(ap, size);
    for (int i = 0; i < size; ++i)
        args.push_back(va_arg(ap, RVal*));
    va_end(ap);
    RVal::Type t = args[0]->type;
    if (t == RVal::Type::Function)
        throw "Cannot concatenate functions";
    for (unsigned i = 1; i < args.size(); ++i) {
        if (args[i]->type != t)
            throw "Types of all c arguments must be the same";
    }
    if (t == RVal::Type::Double) {
        int size = 0;
        for (RVal * v : args)
            size += v->d->size;
        double * result = new double[size];
        int offset = 0;
        for (RVal * v : args) {
            memcpy(result + offset, v->d->data, v->d->size * sizeof(double));
            offset += v->d->size;
        }
        return new RVal(new DoubleVector(result, size));
    } else { // Character
        int size = 0;
        for (RVal * v : args)
            size += v->c->size;
        CharacterVector * result = new CharacterVector(size);
        int offset = 0;
        for (RVal * v : args) {
            memcpy(result->data + offset, v->d->data, v->c->size * sizeof(char));
            offset += v->c->size;
        }
        return new RVal(result);
    }
}





} // extern "C"


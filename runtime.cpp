#include <cassert>
#include <cstdarg>
#include <cstring>
#include <vector>
#include <iostream>
#include <unordered_map>
#include <chrono>

#include "runtime.h"
#include "lexer.h"
#include "parser.h"
#include "compiler.h"
#include "pool.h"

using namespace std;
using namespace rift;

double eval_time;


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

RVal * doubleVectorLiteral(double RVal) {
    return * new DoubleVector(RVal);
}

RVal * characterVectorLiteral(int cpIndex) {
    std::string const & original = Pool::getPoolObject(cpIndex);
    return * new CharacterVector(original.c_str());

}

DoubleVector * doubleFromValue(RVal * v) {
    return static_cast<DoubleVector*>(*v);
}

double scalarFromVector(DoubleVector * v) {
    if (v->size != 1)
        throw "not a scalar";
    return v->data[0];
}

CharacterVector * characterFromValue(RVal *v) {
    return static_cast<CharacterVector*>(*v);
}

RFun * functionFromValue(RVal *v) {
    return static_cast<RFun*>(*v);
}


double doubleGetSingleElement(DoubleVector * from, double index) {
    if (index < 0 or index >= from->size)
        throw "Index out of bounds";
    return from->data[static_cast<unsigned>(index)];
}

DoubleVector * doubleGetElement(DoubleVector * from, DoubleVector * index) {
    unsigned resultSize = index->size;
    double * result = new double[resultSize];
    for (unsigned i = 0; i < resultSize; ++i) {
        double idx = index->data[i];
        if (idx < 0 or idx >= from->size)
            throw "Index out of bounds";
        result[i] = from->data[static_cast<int>(idx)];
    }
    return new DoubleVector(result, resultSize);
}

CharacterVector * characterGetElement(CharacterVector * from, DoubleVector * index) {
    unsigned resultSize = index->size;
    CharacterVector * result = new CharacterVector(resultSize);
    for (unsigned i = 0; i < resultSize; ++i) {
        double idx = index->data[i];
        if (idx < 0 or idx >= from->size)
            throw "Index out of bounds";
        result->data[i] = from->data[static_cast<int>(idx)];
    }
    return result;
}


RVal * genericGetElement(RVal * from, RVal * index) {
    auto i = cast<DoubleVector>(index);
    if (!i) throw "Index vector must be double";
    if (auto fr = cast<DoubleVector>(from))
        return *doubleGetElement(fr, i);
    if (auto fr = cast<CharacterVector>(from)) {
        return *characterGetElement(fr, i);
    }
    throw "Cannot index a function";
}

void doubleSetElement(DoubleVector * target, DoubleVector * index, DoubleVector * RVal) {
    for (unsigned i = 0; i < index->size; ++i) {
        double idx = index->data[i];
        if (idx < 0 or idx >= target->size)
            throw "Index out of bound";
        double val = RVal->data[i % RVal->size];
        target->data[static_cast<int>(idx)] = val;
    }
}

void scalarSetElement(DoubleVector * target, double index, double RVal) {
    if (index < 0 or index >= target->size)
        throw "Index out of bound";
    target->data[static_cast<int>(index)] = RVal;
}

void characterSetElement(CharacterVector * target, DoubleVector * index, CharacterVector * RVal) {
    for (unsigned i = 0; i < index->size; ++i) {
        double  idx = index->data[i];
        if (idx < 0 or idx >= target->size)
            throw "Index out of bound";
        char val = RVal->data[i % RVal->size];
        target->data[static_cast<int>(idx)] = val;
    }
}

void genericSetElement(RVal * target, RVal * index, RVal * value) {
    auto i = cast<DoubleVector>(index);
    if (!i) throw "Index vector must be double";
    if (target->type() != value->type())
        throw "Vector and element must be of same type";
    if (auto t = cast<DoubleVector>(target)) {
        doubleSetElement(t, i, static_cast<DoubleVector*>(*value));
    } else if (auto t = cast<CharacterVector>(target)) {
        characterSetElement(t, i, static_cast<CharacterVector*>(*value));
    } else {
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
    if (lhs->type() != rhs->type())
        throw "Incompatible types for binary operator";
    if (auto l = cast<DoubleVector>(lhs)) {
        return *doubleAdd(l, static_cast<DoubleVector*>(*rhs));
    } else if (auto l = cast<CharacterVector>(lhs)) {
        return *characterAdd(l, static_cast<CharacterVector*>(*rhs));
    } else {
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
    auto l = cast<DoubleVector>(lhs);
    auto r = cast<DoubleVector>(rhs);
    if (!(l && r))
        throw "Invalid types for binary sub";
    return *doubleSub(l, r);
}

DoubleVector * doubleMul(DoubleVector * lhs, DoubleVector * rhs) {
    int resultSize = max(lhs->size, rhs->size);
    double * result = new double[resultSize];
    for (int i = 0; i < resultSize; ++i)
        result[i] = lhs->data[i % lhs->size] * rhs->data[i % rhs->size];
    return new DoubleVector(result, resultSize);
}

RVal * genericMul(RVal * lhs, RVal * rhs) {
    auto l = cast<DoubleVector>(lhs);
    auto r = cast<DoubleVector>(rhs);
    if (!(l && r))
        throw "Invalid types for binary sub";
    return *doubleMul(l, r);
}

DoubleVector * doubleDiv(DoubleVector * lhs, DoubleVector * rhs) {
    int resultSize = max(lhs->size, rhs->size);
    double * result = new double[resultSize];
    for (int i = 0; i < resultSize; ++i)
        result[i] = lhs->data[i % lhs->size] / rhs->data[i % rhs->size];
    return new DoubleVector(result, resultSize);
}

RVal * genericDiv(RVal * lhs, RVal * rhs) {
    auto l = cast<DoubleVector>(lhs);
    auto r = cast<DoubleVector>(rhs);
    if (!(l && r))
        throw "Invalid types for binary sub";
    return *doubleDiv(l, r);
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
    if (lhs->type() != rhs->type())
        return * new DoubleVector(0);

    if (auto l = cast<DoubleVector>(lhs))
        return *doubleEq(l, static_cast<DoubleVector*>(*rhs));
    if (auto l = cast<CharacterVector>(lhs))
        return *characterEq(l, static_cast<CharacterVector*>(*rhs));
    if (auto l = cast<RFun>(lhs))
        return * new DoubleVector(l->code == static_cast<RFun*>(*rhs)->code);

    assert(false);
    return nullptr;
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
    if (lhs->type() != rhs->type())
        return * new DoubleVector(1);

    if (auto l = cast<DoubleVector>(lhs))
        return *doubleNeq(l, static_cast<DoubleVector*>(*rhs));
    if (auto l = cast<CharacterVector>(lhs))
        return *characterNeq(l, static_cast<CharacterVector*>(*rhs));
    if (auto l = cast<RFun>(lhs))
        return * new DoubleVector(l->code != static_cast<RFun*>(*rhs)->code);

    assert(false);
    return nullptr;
}

DoubleVector * doubleLt(DoubleVector * lhs, DoubleVector * rhs) {
    int resultSize = max(lhs->size, rhs->size);
    double * result = new double[resultSize];
    for (int i = 0; i < resultSize; ++i)
        result[i] = lhs->data[i % lhs->size] < rhs->data[i % rhs->size];
    return new DoubleVector(result, resultSize);
}

RVal * genericLt(RVal * lhs, RVal * rhs) {
    auto l = cast<DoubleVector>(lhs);
    auto r = cast<DoubleVector>(rhs);
    if (!(l && r))
        throw "Invalid types for binary sub";

    return *doubleLt(l, r);
}

DoubleVector * doubleGt(DoubleVector * lhs, DoubleVector * rhs) {
    int resultSize = max(lhs->size, rhs->size);
    double * result = new double[resultSize];
    for (int i = 0; i < resultSize; ++i)
        result[i] = lhs->data[i % lhs->size] > rhs->data[i % rhs->size];
    return new DoubleVector(result, resultSize);
}


RVal * genericGt(RVal * lhs, RVal * rhs) {
    auto l = cast<DoubleVector>(lhs);
    auto r = cast<DoubleVector>(rhs);
    if (!(l && r))
        throw "Invalid types for binary sub";
    return *doubleGt(l, r);
}

RVal * createFunction(int index, Environment * env) {
    return * new RFun(Pool::getFunction(index), env);
}

bool toBoolean(RVal * v) {
    if (cast<RFun>(v)) {
        return true;
    } else if (auto c = cast<CharacterVector>(v)) {
        return (c->size > 0) and (c->data[0] != 0);
    } else if (auto d = cast<DoubleVector>(v)) { 
        return (d->size > 0) and (d->data[0] != 0);
    }

    assert(false);
    return false;
}

RVal * call(RVal * callee, unsigned argc, ...) {
    auto f = cast<RFun>(callee);
    if (!f) throw "Not a function!";

    if (f->argsSize != argc) throw "Wrong number of arguments";

    Environment * calleeEnv = new Environment(f->env);
    va_list ap;
    va_start(ap, argc);
    for (unsigned i = 0; i < argc; ++i) {
        // TODO this is pass by reference always!
        auto name = f->args[i];
        auto value = va_arg(ap, RVal*);
        calleeEnv->set(name, value);
    }
    va_end(ap);
    return f->code(calleeEnv);
}

double length(RVal * v) {
    if (auto d = cast<DoubleVector>(v))
        return d->size;
    if (auto c = cast<CharacterVector>(v))
        return c->size;

    if (cast<RFun>(v))
        throw "Cannot determine length of a function";
    throw "Cannot determine length of unknown object";
}

CharacterVector * type(RVal * RVal) {
    switch (RVal->type()) {
    case Type::Double:
        return new CharacterVector("double");
    case Type::Character:
        return new CharacterVector("character");
    case Type::Function:
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
        return * new DoubleVector(0);
    FunPtr f = compile(x);
    auto start = chrono::high_resolution_clock::now();
    RVal * result = f(env);
    auto t = chrono::high_resolution_clock::now() - start;
    eval_time = static_cast<double>(t.count()) / chrono::high_resolution_clock::period::den;
    delete x;
    return result;
}

RVal * characterEval(Environment * env, CharacterVector * RVal) {
    if (RVal->size == 0)
        throw "Cannot evaluate empty character vector";
    return eval(env, RVal->data);
}

RVal * genericEval(Environment * env, RVal * arg) {
    if (auto c = cast<CharacterVector>(arg))
        return characterEval(env, c);

    throw "Only character vectors can be evaluated";
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
        return * new DoubleVector(nullptr, 0);
    std::vector<RVal *> args;
    va_list ap;
    va_start(ap, size);
    for (int i = 0; i < size; ++i)
        args.push_back(va_arg(ap, RVal*));
    va_end(ap);

    Type t = args[0]->type();
    if (t == Type::Function)
        throw "Cannot concatenate functions";

    for (unsigned i = 1; i < args.size(); ++i) {
        if (args[i]->type() != t)
            throw "Types of all c arguments must be the same";
    }

    if (t == Type::Double) {
        size_t size = 0;
        for (RVal * v : args) {
            auto d = static_cast<DoubleVector*>(*v);
            size += d->size;
        }
        double * result = new double[size];
        unsigned offset = 0;
        for (RVal * v : args) {
            auto d = static_cast<DoubleVector*>(*v);
            memcpy(result + offset, d->data, d->size * sizeof(double));
            offset += d->size;
        }
        return * new DoubleVector(result, size);
    } else { // Character
        size_t size = 0;
        for (RVal * v : args) {
            auto c = static_cast<CharacterVector*>(*v);
            size += c->size;
        }
        CharacterVector * result = new CharacterVector(size);
        unsigned offset = 0;
        for (RVal * v : args) {
            auto c = static_cast<CharacterVector*>(*v);
            memcpy(result->data + offset, c->data, c->size * sizeof(char));
            offset += c->size;
        }
        return * result;
    }
}





} // extern "C"


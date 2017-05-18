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
#include "pool.h"
#include "utils/printer.h"

#include "compiler/jit.h"

using namespace std;
using namespace rift;

double eval_time;


extern "C" {

Environment * envCreate(Environment * parent) {
    return Environment::New(parent);
}

RVal * envGet(Environment * env, int symbol) {
    return env->get(symbol);
}

void envSet(Environment * env, int symbol, RVal * RVal) {
    env->set(symbol, RVal);
}

RVal * doubleVectorLiteral(double RVal) {
    return DoubleVector::New({RVal});
}

RVal * characterVectorLiteral(int cpIndex) {
    std::string const & original = Pool::getPoolObject(cpIndex);
    return CharacterVector::New(original.c_str());

}

double scalarFromVector(DoubleVector * v) {
    if (v->size != 1)
        throw "not a scalar";
    return (*v)[0];
}

double doubleGetSingleElement(DoubleVector * from, double index) {
    if (index < 0 or index >= from->size)
        throw "Index out of bounds";
    return (*from)[static_cast<unsigned>(index)];
}

RVal * doubleGetElement(DoubleVector * from, DoubleVector * index) {
    unsigned resultSize = index->size;
    DoubleVector* result = DoubleVector::New(resultSize);
    for (unsigned i = 0; i < resultSize; ++i) {
        double idx = (*index)[i];
        if (idx < 0 or idx >= from->size)
            throw "Index out of bounds";
        (*result)[i] = (*from)[static_cast<int>(idx)];
    }
    return result;
}

RVal * characterGetElement(CharacterVector * from, DoubleVector * index) {
    unsigned resultSize = index->size;
    CharacterVector* result = CharacterVector::New(resultSize);
    for (unsigned i = 0; i < resultSize; ++i) {
        double idx = (*index)[i];
        if (idx < 0 or idx >= from->size)
            throw "Index out of bounds";
        (*result)[i] = (*from)[static_cast<int>(idx)];
    }
    return result;
}


RVal * genericGetElement(RVal * from, RVal * index) {
    auto i = DoubleVector::Cast(index);
    if (!i) throw "Index vector must be double";
    if (auto fr = DoubleVector::Cast(from))
        return doubleGetElement(fr, i);
    if (auto fr = CharacterVector::Cast(from)) {
        return characterGetElement(fr, i);
    }
    throw "Cannot index a function";
}

void doubleSetElement(DoubleVector * target, DoubleVector * index, DoubleVector * RVal) {
    for (unsigned i = 0; i < index->size; ++i) {
        double idx = (*index)[i];
        if (idx < 0 or idx >= target->size)
            throw "Index out of bound";
        double val = (*RVal)[i % RVal->size];
        (*target)[static_cast<int>(idx)] = val;
    }
}

void scalarSetElement(DoubleVector * target, double index, double RVal) {
    if (index < 0 or index >= target->size)
        throw "Index out of bound";
    (*target)[static_cast<int>(index)] = RVal;
}

void characterSetElement(CharacterVector * target, DoubleVector * index, CharacterVector * RVal) {
    for (unsigned i = 0; i < index->size; ++i) {
        double  idx = (*index)[i];
        if (idx < 0 or idx >= target->size)
            throw "Index out of bound";
        char val = (*RVal)[i % RVal->size];
        (*target)[static_cast<int>(idx)] = val;
    }
}

void genericSetElement(RVal * target, RVal * index, RVal * value) {
    auto i = DoubleVector::Cast(index);
    if (!i) throw "Index vector must be double";
    if (target->type != value->type)
        throw "Vector and element must be of same type";
    if (auto t = DoubleVector::Cast(target)) {
        doubleSetElement(t, i, static_cast<DoubleVector*>(value));
    } else if (auto t = CharacterVector::Cast(target)) {
        characterSetElement(t, i, static_cast<CharacterVector*>(value));
    } else {
        throw "Cannot index a function";
    }
}

RVal * doubleAdd(DoubleVector * lhs, DoubleVector * rhs) {
    int resultSize = max(lhs->size, rhs->size);
    DoubleVector* res = DoubleVector::New(resultSize);
    for (int i = 0; i < resultSize; ++i)
        (*res)[i] = (*lhs)[i % lhs->size] + (*rhs)[i % rhs->size];
    return res;
}

RVal * characterAdd(CharacterVector * lhs, CharacterVector * rhs) {
    int resultSize = lhs->size + rhs->size;
    CharacterVector* result = CharacterVector::New(resultSize);
    memcpy(result->data, lhs->data, lhs->size);
    memcpy(result->data + lhs->size, rhs->data, rhs->size);
    return result;
}


RVal * genericAdd(RVal * lhs, RVal * rhs) {
    if (lhs->type != rhs->type)
        throw "Incompatible types for binary operator";
    if (auto l = DoubleVector::Cast(lhs)) {
        return doubleAdd(l, static_cast<DoubleVector*>(rhs));
    } else if (auto l = CharacterVector::Cast(lhs)) {
        return characterAdd(l, static_cast<CharacterVector*>(rhs));
    } else {
        throw "Invalid types for binary add";
    }
}

RVal * doubleSub(DoubleVector * lhs, DoubleVector * rhs) {
    int resultSize = max(lhs->size, rhs->size);
    DoubleVector* result = DoubleVector::New(resultSize);
    for (int i = 0; i < resultSize; ++i)
        (*result)[i] = (*lhs)[i % lhs->size] - (*rhs)[i % rhs->size];
    return result;
}



RVal * genericSub(RVal * lhs, RVal * rhs) {
    auto l = DoubleVector::Cast(lhs);
    auto r = DoubleVector::Cast(rhs);
    if (!(l && r))
        throw "Invalid types for binary sub";
    return doubleSub(l, r);
}

RVal * doubleMul(DoubleVector * lhs, DoubleVector * rhs) {
    int resultSize = max(lhs->size, rhs->size);
    DoubleVector* result = DoubleVector::New(resultSize);
    for (int i = 0; i < resultSize; ++i)
        (*result)[i] = (*lhs)[i % lhs->size] * (*rhs)[i % rhs->size];
    return result;
}

RVal * genericMul(RVal * lhs, RVal * rhs) {
    auto l = DoubleVector::Cast(lhs);
    auto r = DoubleVector::Cast(rhs);
    if (!(l && r))
        throw "Invalid types for binary sub";
    return doubleMul(l, r);
}

RVal * doubleDiv(DoubleVector * lhs, DoubleVector * rhs) {
    int resultSize = max(lhs->size, rhs->size);
    DoubleVector* result = DoubleVector::New(resultSize);
    for (int i = 0; i < resultSize; ++i)
        (*result)[i] = (*lhs)[i % lhs->size] / (*rhs)[i % rhs->size];
    return result;
}

RVal * genericDiv(RVal * lhs, RVal * rhs) {
    auto l = DoubleVector::Cast(lhs);
    auto r = DoubleVector::Cast(rhs);
    if (!(l && r))
        throw "Invalid types for binary sub";
    return doubleDiv(l, r);
}

RVal * doubleEq(DoubleVector * lhs, DoubleVector * rhs) {
    int resultSize = max(lhs->size, rhs->size);
    DoubleVector* result = DoubleVector::New(resultSize);
    for (int i = 0; i < resultSize; ++i)
        (*result)[i] = (*lhs)[i % lhs->size] == (*rhs)[i % rhs->size];
    return result;
}

RVal * characterEq(CharacterVector * lhs, CharacterVector * rhs) {
    int resultSize = max(lhs->size, rhs->size);
    DoubleVector* result = DoubleVector::New(resultSize);
    for (int i = 0; i < resultSize; ++i)
        (*result)[i] = (*lhs)[i % lhs->size] == (*rhs)[i % rhs->size];
    return result;
}

RVal * genericEq(RVal * lhs, RVal * rhs) {
    if (lhs->type != rhs->type)
        return DoubleVector::New({0});

    if (auto l = DoubleVector::Cast(lhs))
        return doubleEq(l, static_cast<DoubleVector*>(rhs));
    if (auto l = CharacterVector::Cast(lhs))
        return characterEq(l, static_cast<CharacterVector*>(rhs));
    if (auto l = RFun::Cast(lhs))
        return DoubleVector::New(
                {static_cast<double>(l->code == static_cast<RFun*>(rhs)->code)});

    assert(false);
    return nullptr;
}

RVal * doubleNeq(DoubleVector * lhs, DoubleVector * rhs) {
    int resultSize = max(lhs->size, rhs->size);
    DoubleVector* result = DoubleVector::New(resultSize);
    for (int i = 0; i < resultSize; ++i)
        (*result)[i] = (*lhs)[i % lhs->size] != (*rhs)[i % rhs->size];
    return result;
}

RVal * characterNeq(CharacterVector * lhs, CharacterVector * rhs) {
    int resultSize = max(lhs->size, rhs->size);
    DoubleVector* result = DoubleVector::New(resultSize);
    for (int i = 0; i < resultSize; ++i)
        (*result)[i] = (*lhs)[i % lhs->size] == (*rhs)[i % rhs->size];
    return result;
}

RVal * genericNeq(RVal * lhs, RVal * rhs) {
    if (lhs->type != rhs->type)
        return DoubleVector::New({1});

    if (auto l = DoubleVector::Cast(lhs))
        return doubleNeq(l, static_cast<DoubleVector*>(rhs));
    if (auto l = CharacterVector::Cast(lhs))
        return characterNeq(l, static_cast<CharacterVector*>(rhs));
    if (auto l = RFun::Cast(lhs))
        return DoubleVector::New(
                {static_cast<double>(l->code != static_cast<RFun*>(rhs)->code)});

    assert(false);
    return nullptr;
}

RVal * doubleLt(DoubleVector * lhs, DoubleVector * rhs) {
    int resultSize = max(lhs->size, rhs->size);
    DoubleVector* result = DoubleVector::New(resultSize);
    for (int i = 0; i < resultSize; ++i)
        (*result)[i] = (*lhs)[i % lhs->size] < (*rhs)[i % rhs->size];
    return result;
}

RVal * genericLt(RVal * lhs, RVal * rhs) {
    auto l = DoubleVector::Cast(lhs);
    auto r = DoubleVector::Cast(rhs);
    if (!(l && r))
        throw "Invalid types for binary sub";

    return doubleLt(l, r);
}

RVal * doubleGt(DoubleVector * lhs, DoubleVector * rhs) {
    int resultSize = max(lhs->size, rhs->size);
    DoubleVector* result = DoubleVector::New(resultSize);
    for (int i = 0; i < resultSize; ++i)
        (*result)[i] = (*lhs)[i % lhs->size] > (*rhs)[i % rhs->size];
    return result;
}


RVal * genericGt(RVal * lhs, RVal * rhs) {
    auto l = DoubleVector::Cast(lhs);
    auto r = DoubleVector::Cast(rhs);
    if (!(l && r))
        throw "Invalid types for binary sub";
    return doubleGt(l, r);
}

RVal * createFunction(int index, Environment * env) {
    return Pool::getFunction(index)->close(env);
}

bool toBoolean(RVal * v) {
    if (RFun::Cast(v)) {
        return true;
    } else if (auto c = CharacterVector::Cast(v)) {
        return (c->size > 0) and ((*c)[0] != 0);
    } else if (auto d = DoubleVector::Cast(v)) {
        return (d->size > 0) and ((*d)[0] != 0);
    }

    assert(false);
    return false;
}

RVal * call(RVal * callee, unsigned argc, ...) {
    auto f = RFun::Cast(callee);
    if (!f) throw "Not a function!";

    if (f->nargs() != argc) throw "Wrong number of arguments";

    Bindings * calleeBindings = Bindings::New(argc);
    va_list ap;
    va_start(ap, argc);
    for (unsigned i = 0; i < argc; ++i) {
        // TODO this is pass by reference always!
        auto name = (*f->args)[i];
        auto value = va_arg(ap, RVal*);
        calleeBindings->binding[i].symbol = name;
        calleeBindings->binding[i].value = value;
    }
    calleeBindings->size = argc;
    va_end(ap);
    Environment * calleeEnv = Environment::New(f->env, calleeBindings);
    return f->code(calleeEnv);
}

double length(RVal * v) {
    if (auto d = DoubleVector::Cast(v))
        return d->size;
    if (auto c = CharacterVector::Cast(v))
        return c->size;

    if (RFun::Cast(v))
        throw "Cannot determine length of a function";
    throw "Cannot determine length of unknown object";
}

RVal * type(RVal * v) {
    switch (v->type) {
    case Type::Double:
        return CharacterVector::New("double");
    case Type::Character:
        return CharacterVector::New("character");
    case Type::Function:
        return CharacterVector::New("function");
    default:
        return nullptr;
    }
}

RVal * eval(Environment * env, char const * value) {
    std::string s(value);
    Parser p;
    ast::Fun * x = new ast::Fun(p.parse(s));
    if (x->body->body.empty())
        return DoubleVector::New({0});

    if (DEBUG) {
        std::cout << "AST:" << std::endl;
        Printer::print(x);
        std::cout << std::endl;
    }

    FunPtr f = JIT::compile(x);
    auto start = chrono::high_resolution_clock::now();
    RVal * result = f(env);
    auto t = chrono::high_resolution_clock::now() - start;
    eval_time = static_cast<double>(t.count()) / chrono::high_resolution_clock::period::den;
    JIT::removeLastModule();
    delete x;
    return result;
}

RVal * characterEval(Environment * env, CharacterVector * RVal) {
    if (RVal->size == 0)
        throw "Cannot evaluate empty character vector";
    return eval(env, RVal->data);
}

RVal * genericEval(Environment * env, RVal * arg) {
    if (auto c = CharacterVector::Cast(arg))
        return characterEval(env, c);

    throw "Only character vectors can be evaluated";
}

RVal * doublec(int size, ...) {
    std::vector<DoubleVector *> args;
    va_list ap;
    va_start(ap, size);
    for (int i = 0; i < size; ++i)
        args.push_back(va_arg(ap, DoubleVector*));
    va_end(ap);
    size = 0;
    for (DoubleVector * v : args)
        size += v->size;
    DoubleVector* result = DoubleVector::New(size);
    int offset = 0;
    for (DoubleVector * v : args) {
        memcpy(result->data + offset, v->data, v->size * sizeof(double));
        offset += v->size;
    }
    return result;
}

RVal * characterc(int size, ...) {
    std::vector<CharacterVector *> args;
    va_list ap;
    va_start(ap, size);
    for (int i = 0; i < size; ++i)
        args.push_back(va_arg(ap, CharacterVector*));
    va_end(ap);
    size = 0;
    for (CharacterVector * v : args)
        size += v->size;
    CharacterVector * result = CharacterVector::New(size);
    int offset = 0;
    for (CharacterVector * v : args) {
        memcpy(result->data + offset, v->data, v->size * sizeof(char));
        offset += v->size;
    }
    return result;
}

RVal * c(int size, ...) {
    if (size == 0)
        return DoubleVector::New({});
    std::vector<RVal *> args;
    va_list ap;
    va_start(ap, size);
    for (int i = 0; i < size; ++i)
        args.push_back(va_arg(ap, RVal*));
    va_end(ap);

    Type t = args[0]->type;
    if (t == Type::Function)
        throw "Cannot concatenate functions";

    for (unsigned i = 1; i < args.size(); ++i) {
        if (args[i]->type != t)
            throw "Types of all c arguments must be the same";
    }

    if (t == Type::Double) {
        size_t size = 0;
        for (RVal * v : args) {
            auto d = static_cast<DoubleVector*>(v);
            size += d->size;
        }
        DoubleVector* result = DoubleVector::New(size);
        unsigned offset = 0;
        for (RVal * v : args) {
            auto d = static_cast<DoubleVector*>(v);
            memcpy(result->data + offset, d->data, d->size * sizeof(double));
            offset += d->size;
        }
        return result;
    } else { // Character
        size_t size = 0;
        for (RVal * v : args) {
            auto c = static_cast<CharacterVector*>(v);
            size += c->size;
        }
        CharacterVector * result = CharacterVector::New(size);
        unsigned offset = 0;
        for (RVal * v : args) {
            auto c = static_cast<CharacterVector*>(v);
            memcpy(result->data + offset, c->data, c->size * sizeof(char));
            offset += c->size;
        }
        return result;
    }
}

} // extern "C"

#ifndef RUNTIME_H
#define RUNTIME_H

#include <llvm/IR/Verifier.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/Support/raw_ostream.h>
#include "llvm/Analysis/Passes.h"
#include <llvm/Support/raw_os_ostream.h>

#include "ast.h"

struct Value;

struct CharacterVector {
    char * data;
    unsigned size;
    CharacterVector(char * data, int size):
        data(data),
        size(size) {
    }
    ~CharacterVector() {
        delete [] data;
    }
    void print(std::ostream & s) {
        s << data;
    }

    static CharacterVector * copy(char const * from) {
        std::string s(from);
        char * x = new char[s.size()];
        memcpy(x, s.c_str(), s.size());
        return new CharacterVector(x, s.size());
    }
};

struct DoubleVector {
    double * data;
    unsigned size;
    DoubleVector(double value):
        data(new double[1]),
        size(1) {
        data[0] = value;
    }
    DoubleVector(double * data, unsigned size):
        data(data),
        size(size) {
    }

    ~DoubleVector() {
        delete [] data;
    }
    void print(std::ostream & s) {
        for (int i = 0; i < size; ++i)
            s << data[i] << " ";
    }
};


struct Binding2 {
    int symbol;
    Value * value;
};

struct Environment {
    Environment * parent;
    Binding2 * bindings;
    int size;
    Environment(Environment * parent):
        parent(parent),
        bindings(nullptr),
        size(0) {
    }
    Value * get(int symbol) {
        for (int i = 0; i < size; ++i)
            if (bindings[i].symbol == symbol)
                return bindings[i].value;
        if (parent != nullptr)
            return parent->get(symbol);
        else
            throw "Variable not found";
    }

    void set(int symbol, Value * value) {
        for (int i = 0; i < size; ++i)
            if (bindings[i].symbol == symbol) {
                bindings[i].value = value;
                return;
            }
        Binding2 * n = new Binding2[size + 1];
        memcpy(n, bindings, size * sizeof(Binding2));
        delete [] bindings;
        bindings = n;
        bindings[size].symbol = symbol;
        bindings[size].value = value;
        ++size;
    }
};

typedef Value * (*FunPtr)(Environment *);

struct Function {
    Environment * env;
    FunPtr code;
    llvm::Function * bitcode;
    int * args;
    unsigned argsSize;
    Function(rift::ast::Fun * fun, llvm::Function * bitcode):
        env(nullptr),
        code(nullptr),
        bitcode(bitcode),
        args(fun->argumentCount() == 0 ? nullptr : new int[fun->argumentCount()]),
        argsSize(fun->argumentCount()) {
        unsigned i = 0;
        for (rift::ast::Var * arg : *fun)
            args[i++] = arg->index();
    }

    Function(Function * f, Environment * e):
        env(e),
        code(f->code),
        bitcode(f->bitcode),
        args(f->args),
        argsSize(f->argsSize) {
    }

    ~Function() {
        // do not delete args, or env, or anything else
    }

    void print(std::ostream & s) {
        llvm::raw_os_ostream ss(s);
        bitcode->print(ss);
    }

};

struct Value {
    enum class Type {
        Double,
        Character,
        Function,
    };
    Type type;
    union {
        DoubleVector * d;
        CharacterVector * c;
        Function * f;
    };

    Value(DoubleVector * d):
        type(Type::Double),
        d(d) {
    }

    Value(CharacterVector * c):
        type(Type::Character),
        c(c) {
    }

    Value(Function * f):
        type(Type::Function),
        f(f) {
    }
    ~Value() {
        switch (type) {
        case Type::Double:
            delete d;
            break;
        case Type::Character:
            delete c;
            break;
        case Type::Function:
            delete f;
            break;
        }
    }
    void print(std::ostream & s) {
        switch (type) {
        case Type::Double:
            d->print(s);
            break;
        case Type::Character:
            c->print(s);
            break;
        case Type::Function:
            f->print(s);
            break;
        }
    }
};

extern "C" {

/** Creates new environment */
Environment * envCreate(Environment * parent);

Value * envGet(Environment * env, int symbol);
Value * envSet(Environment * env, int symbol, Value * value);


DoubleVector * doubleVectorLiteral(double value);
CharacterVector * characterVectorLiteral(int cpIndex);




Value * fromDoubleVector(DoubleVector * from);
Value * fromCharacterVector(CharacterVector * from);
Value * fromFunction(Function * from);

DoubleVector * doubleGetElement(DoubleVector * from, DoubleVector * index);
CharacterVector * characterGetElement(CharacterVector * from, DoubleVector * index);
Value * genericGetElement(Value * from, Value * index);
void doubleSetElement(DoubleVector * target, DoubleVector * index, DoubleVector * value);
void characterSetElement(CharacterVector * target, DoubleVector * index, CharacterVector * value);
void genericSetElement(Value * target, Value * index, Value * value);

DoubleVector * doubleAdd(DoubleVector * lhs, DoubleVector * rhs);
CharacterVector * characterAdd(CharacterVector * lhs, CharacterVector * rhs);
Value * genericAdd(Value * lhs, Value * rhs);
DoubleVector * doubleSub(DoubleVector * lhs, DoubleVector * rhs);
Value * genericSub(Value * lhs, Value * rhs);
DoubleVector * doubleMul(DoubleVector * lhs, DoubleVector * rhs);
Value * genericMul(Value * lhs, Value * rhs);
DoubleVector * doubleDiv(DoubleVector * lhs, DoubleVector * rhs);
Value * genericDiv(Value * lhs, Value * rhs);
DoubleVector * doubleEq(DoubleVector * lhs, DoubleVector * rhs);
DoubleVector * characterEq(CharacterVector * lhs, CharacterVector * rhs);
DoubleVector * functionEq(Function * lhs, Function * rhs);
Value * genericEq(Value * lhs, Value * rhs);
DoubleVector * doubleNeq(DoubleVector * lhs, DoubleVector * rhs);
DoubleVector * characterNeq(CharacterVector * lhs, CharacterVector * rhs);
DoubleVector * functionNeq(Function * lhs, Function * rhs);
Value * genericNeq(Value * lhs, Value * rhs);
DoubleVector * doubleLt(DoubleVector * lhs, DoubleVector * rhs);
Value * genericLt(Value * lhs, Value * rhs);
DoubleVector * doubleGt(DoubleVector * lhs, DoubleVector * rhs);
Value * genericGt(Value * lhs, Value * rhs);

Function * createFunction(int index, Environment * env);

bool toBoolean(Value * value);

Value * call(Value * callee, Environment * parent, int argc, ...);

double length(Value * value);
CharacterVector * type(Value * value);
Value * eval(Environment * env, char const * value);
Value * characterEval(Environment * env, CharacterVector * value);
Value * genericEval(Environment * env, Value * value);
Value * c(int size, ...);

}

namespace rift {
class Runtime {
public:
    static unsigned functionsCount() {
        return f_.size();
    }


    static Function * getFunction(int index) {
        return f_[index];
    }

    static int addFunction(ast::Fun * fun, llvm::Function * bitcode) {
        Function * f = new Function(fun, bitcode);
        f_.push_back(f);
        return f_.size() - 1;
    }

private:

    static std::vector<Function *> f_;
};


}

#endif // RUNTIME_H


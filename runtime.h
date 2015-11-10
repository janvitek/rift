#pragma once
#ifndef RUNTIME_H
#define RUNTIME_H

#include <ciso646>
#include "llvm.h"
#include "ast.h"

/** Value: forward declaration 
*/
struct RVal;

/** Character vector: strings of variable size. They are null
    terminated. Size excludes the trailing terminator.
*/
struct CharacterVector {
    char * data;

    unsigned size;

    /** Creates a CV from given data and size. Takes ownership of the data.
    */
    CharacterVector(size_t size):
        data(new char[size + 1]),
        size(size) {
        data[size] = 0;
    }

    /** Creates the character vector from existing null terminated string. 
        Creates a copy of the string internally. 
    */
    CharacterVector(char const * from) {
        size = strlen(from);
        data = new char[size + 1];
        memcpy(data, from, size + 1);
    }

    /** Deletes data. */
    ~CharacterVector() {
        delete [] data;
    }

    /** Prints to given stream. */
    void print(std::ostream & s) {
        s << data;
    }

};

/** A Double vector consists of an array of doubles and a size.
*/
struct DoubleVector {
    
    double * data;

    unsigned size;
    
    /** Creates vector of length one.
     */
    DoubleVector(double value):
        data(new double[1]),
        size(1) {
        data[0] = value;
    }

    /** Creates vector from array. The array is owned by the vector.
    */
    DoubleVector(double * data, unsigned size):
        data(data),
        size(size) {
    }

    /** Creates vector from doubles.
     */
    DoubleVector(std::initializer_list<double> d) {
        size = d.size();
        data = new double[size];
        unsigned i = 0;
        for (double dd : d)
            data[i++] = dd;
    }

    /** Deletes data. 
     */
    ~DoubleVector() {
        delete [] data;
    }

    /** Prints to given stream. 
     */
    void print(std::ostream & s) {
        for (unsigned i = 0; i < size; ++i)
            s << data[i] << " ";
    }
};

/** Binding for the environment. 

Binding is a pair of symbol and corresponding Value. 
*/
struct Binding {
    rift::Symbol symbol;
    RVal * value;
};

/** Rift Environment. 

Environment represents function's local variables as well as a pointer to the parent environment, i.e. the environment in which the function was declared. 

*/
struct Environment {

    /** Parent environment. 

    Nullptr if top environment. 
     */
    Environment * parent;
    
    /** Array of active bindings. 
     */
    Binding * bindings;

    /** Size of the bindings array. 
     */
    int size;
    
    /** Creates new environment with the given parent. 
     */
    Environment(Environment * parent):
        parent(parent),
        bindings(nullptr),
        size(0) {
    }

    /** Returns the value corresponding to given Symbol. 

    If the symbol is not found in current bindings, recursively searches parent environments as well. 
     */
    RVal * get(rift::Symbol symbol) {
        for (int i = 0; i < size; ++i)
            if (bindings[i].symbol == symbol)
                return bindings[i].value;
        if (parent != nullptr)
            return parent->get(symbol);
        else
            throw "Variable not found";
    }

    /** Assigns given value to symbol. 
    
    If the symbol already exists it the environment, updates its value, otherwise creates new binding for the symbol and attaches it to the value. 
     */
    void set(rift::Symbol symbol, RVal * value) {
        for (int i = 0; i < size; ++i)
            if (bindings[i].symbol == symbol) {
                bindings[i].value = value;
                return;
            }
        Binding * n = new Binding[size + 1];
        memcpy(n, bindings, size * sizeof(Binding));
        delete [] bindings;
        bindings = n;
        bindings[size].symbol = symbol;
        bindings[size].value = value;
        ++size;
    }
};

/** Pointer to Rift function. These functions all takes Environment* and
    return an RVal*.
 */
typedef RVal * (*FunPtr)(Environment *);

/** Rift Function.  Each function contains an environment, compiled code,
    bitcode, an argument list, and an arity.  The bitcode and argument names
    are there for debugging purposes.
 */
struct RFun {

    Environment * env;

    FunPtr code;
    
    llvm::Function * bitcode;

    rift::Symbol * args;
    
    unsigned argsSize;
    
    /** Create a Rift function. A new argument list is needed, everything
	else is shared.
    */
    RFun(rift::ast::Fun * fun, llvm::Function * bitcode):
        env(nullptr),
        code(nullptr),
        bitcode(bitcode),
        args(fun->args.size()==0 ? nullptr : new int[fun->args.size()]),
        argsSize(fun->args.size()) {
        unsigned i = 0;
        for (rift::ast::Var * arg : fun->args)
            args[i++] = arg->symbol;
    }

    /** Create a closure by copying function f and binding it to environment
	e. Arguments are shared.
     */
    RFun(RFun * f, Environment * e):
        env(e),
        code(f->code),
        bitcode(f->bitcode),
        args(f->args),
        argsSize(f->argsSize) {
    }
  
  /* Args and env are shared, they are not deleted. */
    ~RFun() { }

    /** Prints bitcode to given stream.  */
    void print(std::ostream & s) {
        llvm::raw_os_ostream ss(s);
        bitcode->print(ss);
    }

};

/** A Rift Value. All values have a type tage and can point either of a
    vector of doubles or chararcters, or a function.
*/
struct RVal {
    enum class Type {
        Double,
        Character,
        Function,
    };

    Type type;

    union {
        DoubleVector * d;
        CharacterVector * c;
        RFun * f;
    };

    /** Creates a boxed double vector from given numbers.   */
    RVal(std::initializer_list<double> d) :
        type(Type::Double),
        d(new DoubleVector(d)) {
    }

    /** Creates a boxed character vector from given string.    */
    RVal(char const * c) :
        type(Type::Character),
        c(new CharacterVector(c)) {
    }

    /** Boxes given DoubleVector. Takes ownership of the vector. */
    RVal(DoubleVector * d):
        type(Type::Double),
        d(d) {
    }

    /** Boxes given CharacterVector. Takes ownership of the vector.  */
    RVal(CharacterVector * c):
        type(Type::Character),
        c(c) {
    }

    /** Boxes given Function. Takes ownership of the vector. */
    RVal(RFun * f):
        type(Type::Function),
        f(f) {
    }

    /** Deletes the boxed value.  */
    ~RVal() {
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

    /** Prints to given stream.  */
    void print(std::ostream & s) const {
        switch (type) {
        case Type::Double:    d->print(s); break;
        case Type::Character: c->print(s); break;
        case Type::Function:  f->print(s); break;
        }
    }

    /** Compares values for equality. Only use in tests.  */
    bool operator == (RVal const & other) {
        if (type != other.type)  return false;
        switch (type) {
            case Type::Double: {
                if (d->size != other.d->size) return false;
                for (unsigned i = 0; i < d->size; ++i)
                    if (d->data[i] != other.d->data[i]) return false;
                return true;
            }
            case Type::Character: {
                if (c->size != other.c->size) return false;
                for (unsigned i = 0; i < c->size; ++i)
                    if (c->data[i] != other.c->data[i]) return false;
                return true;
            }
            default: // For functions...
	      return f == other.f;
        }
    }

    /** Compares values for inequality. Only used in tests.    */
    bool operator != (RVal const & other) {
        if (type != other.type)   return true;
        switch (type) {
            case Type::Double: {
                if (d->size != other.d->size) return true;
                for (unsigned i = 0; i < d->size; ++i)
                    if (d->data[i] != other.d->data[i]) return true;
                return false;
            }
            case Type::Character: {
                if (c->size != other.c->size) return true;
                for (unsigned i = 0; i < c->size; ++i)
                    if (c->data[i] != other.c->data[i]) return true;
                return false;
            }
            default: // Functions
                return f != other.f;
        }
    }
};

/** Standard C++ printing support. */
inline std::ostream & operator << (std::ostream & s, RVal const & value) {
    value.print(s);
    return s;
}

/** Functions are defined "extern C" to avoid exposing C++ name mangling to
    LLVM. Dealing with C++ would make the compiler less readable.

    For better performance, one could obtain the bitcode of these functions
    and inline that at code generation time.
*/
extern "C" {

/** Creates new environment from parent. */
Environment * envCreate(Environment * parent);

/** Get value of symbol in env or its parent. Throw if not found. */
RVal * envGet(Environment * env, int symbol);

/** Binds symbol to the value in env. */
void envSet(Environment * env, int symbol, RVal * value);

/** Creates a double vector from the literal. */
DoubleVector * doubleVectorLiteral(double value);

/** Creates a CV from the literal at cpIndex in the constant pool */
CharacterVector * characterVectorLiteral(int cpIndex);

/** Boxes double vector. */
RVal * fromDoubleVector(DoubleVector * from);

/** Boxes character vector. */
RVal * fromCharacterVector(CharacterVector * from);

/** Boxes function. */
RVal * fromFunction(RFun * from);

/** Returns the value at index.  */
RVal * genericGetElement(RVal * from, RVal * index);

/** Sets the value at index.  */
void genericSetElement(RVal * target, RVal * index, RVal * value);

/** Adds doubles and concatenates strings. */
RVal * genericAdd(RVal * lhs, RVal * rhs);

/** Subtracts doubles, or raises an error. */
RVal * genericSub(RVal * lhs, RVal * rhs);

/** Multiplies doubles, or raises an error. */
RVal * genericMul(RVal * lhs, RVal * rhs);

/** Divides  doubles, or raises an error. */
RVal * genericDiv(RVal * lhs, RVal * rhs);

/** Compares values for equality. */
RVal * genericEq(RVal * lhs, RVal * rhs);

/** Compares values for inequality */
RVal * genericNeq(RVal * lhs, RVal * rhs);

/** Compares doubles using '<', or raises an error. */
RVal * genericLt(RVal * lhs, RVal * rhs);

/** Compares doubles using '>', or raises an  errror. */
RVal * genericGt(RVal * lhs, RVal * rhs);

/** Creates a function and binds it to env. Functions are identified by an
    index assigned at compile time.
 */
RFun * createFunction(int index, Environment * env);

/** Given a value, converts it to a boolean. Used in branches. */
bool toBoolean(RVal * value);

/** Calls function with argc arguments.  argc must match the arity of the
    function. A new env is create for the callee and populated by the
    arguments.
 */
RVal * call(RVal * callee, unsigned argc, ...);

/** Returns the length of a vector.  */
double length(RVal * value);

/** Returns the type of a value as a character vector: 'double',
    'character', or 'function'.
 */
CharacterVector * type(RVal * value);

/** Evaluates character vector in env. */
RVal * eval(Environment * env, char const * value);

  /** Evaluates value, raising an error if it is not a characer vector. */
RVal * genericEval(Environment * env, RVal * value);

/** Creates a vector, raising an error if they are not of the same type, or
    if there is a function among them.
 */
RVal * c(int size, ...);
}
#endif // RUNTIME_H

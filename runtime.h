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
    CharacterVector(int size):
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

/** Pointer to rift functions compiled to native code, i.e. a function which takes Environment * as an argument and returns Value *. 
 */
typedef RVal * (*FunPtr)(Environment *);

/** Function object. 

Contains the environment in which the function has been created, i.e. parent environment of all environments of the function itself, pointer to the native code, the llvm function itself for debugging purposes such as displaying the bitcode and the argument names and argument size so that the arguments can be matched at runtime. 
 */
struct Function {

    Environment * env;

    FunPtr code;
    
    llvm::Function * bitcode;

    rift::Symbol * args;
    
    unsigned argsSize;
    
    /** Creates the function object from given function definition AST (this is where we take the arguments from) and llvm function bitcode. 

    Keeps all other fields empty. Hard copies the arguments.
    */
    Function(rift::ast::Fun * fun, llvm::Function * bitcode):
        env(nullptr),
        code(nullptr),
        bitcode(bitcode),
        args(fun->args.size() == 0 ? nullptr : new int[fun->args.size()]),
        argsSize(fun->args.size()) {
        unsigned i = 0;
        for (rift::ast::Var * arg : fun->args)
            args[i++] = arg->symbol;
    }

    /** Takes a function object and creates a copy of it binding it to environment, creating a closure. 

    Args are shallow copied. 
     */
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

    /** Prints the function to given stream. 
    
    Prints the llvm bitcode. 
    */
    void print(std::ostream & s) {
        llvm::raw_os_ostream ss(s);
        bitcode->print(ss);
    }

};

/** Generic value. 

A value comprises of the type specifier (double vector, character vector, or function) and union of pointers to the respective classes. 
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
        ::Function * f;
    };

    /** Creates a boxed double vector from given numbers. 
     */
    RVal(std::initializer_list<double> d) :
        type(Type::Double),
        d(new DoubleVector(d)) {
    }

    /** Creates a boxed character vector from given string. 
     */
    RVal(char const * c) :
        type(Type::Character),
        c(new CharacterVector(c)) {
    }

    /** Boxes given DoubleVector into a Value. 

    Takes ownership of the vector. 
     */
    RVal(DoubleVector * d):
        type(Type::Double),
        d(d) {
    }

    /** Boxes given CharacterVector into a Value.

    Takes ownership of the vector.
    */
    RVal(CharacterVector * c):
        type(Type::Character),
        c(c) {
    }

    /** Boxes given Function into a Value.

    Takes ownership of the vector.
    */
    RVal(::Function * f):
        type(Type::Function),
        f(f) {
    }

    /** Deletes the value, deleting its boxed element based on value's type. 
     */
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

    /** Prints the value to given stream. 
     */
    void print(std::ostream & s) const {
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

    /** Compares two values for equality. 
    
    The operator is only expected to be used by the tests. rift should use the genericEq() function for Value comparisons. 
     */
    bool operator == (RVal const & other) {
        if (type != other.type)
            return false;
        switch (type) {
            case Type::Double: {
                if (d->size != other.d->size)
                    return false;
                for (unsigned i = 0; i < d->size; ++i)
                    if (d->data[i] != other.d->data[i])
                        return false;
                return true;
            }
            case Type::Character: {
                if (c->size != other.c->size)
                    return false;
                for (unsigned i = 0; i < c->size; ++i)
                    if (c->data[i] != other.c->data[i])
                        return false;
                return true;
            }
            case Type::Function:
            default:
                // just to silence warnings
                return f == other.f;
        }
    }

    /** Compares two values for inequality.

    The operator is only expected to be used by the tests. rift should use the genericNeq() function for Value comparisons.
    */
    bool operator != (RVal const & other) {
        if (type != other.type)
            return true;
        switch (type) {
            case Type::Double: {
                if (d->size != other.d->size)
                    return true;
                for (unsigned i = 0; i < d->size; ++i)
                    if (d->data[i] != other.d->data[i])
                        return true;
                return false;
            }
            case Type::Character: {
                if (c->size != other.c->size)
                    return true;
                for (unsigned i = 0; i < c->size; ++i)
                    if (c->data[i] != other.c->data[i])
                        return true;
                return false;
            }
            case Type::Function:
            default: // just to silence warnings
                return f != other.f;
        }
    }
};

/** Standard C++ printing support. 
 */
inline std::ostream & operator << (std::ostream & s, RVal const & value) {
    value.print(s);
    return s;
}

/** Functions available to the rift runtime. While the above mentioned objects and their methods could have been used, the C++ name mangling and other peculiarities of the OOP would make the llvm's code very complex. 

Therefore all interaction with the runtime objects is done by these functions with rather simple interface. The additional level of indirection this creates can be easily elliminated by obtaining the bitcode of the functions using clang and then letting LLVM inline them directly into the code. 
*/
extern "C" {

/** Creates new environment using given parent. 
*/
Environment * envCreate(Environment * parent);

/** Returns value of given symbol in the specified environment or its parents. 

Raises an error if not found. 
 */
RVal * envGet(Environment * env, int symbol);

/** Binds given symbol to the value in the specified environment. 
 */
void envSet(Environment * env, int symbol, RVal * value);

/** Creates a double vector from the literal. 
 */
DoubleVector * doubleVectorLiteral(double value);

/** Creates a character vector from the literal. 
 */
CharacterVector * characterVectorLiteral(int cpIndex);

/** Boxes double vector into a Value. 
 */
RVal * fromDoubleVector(DoubleVector * from);

/** Boxes character vector into a Value. 
 */
RVal * fromCharacterVector(CharacterVector * from);

/** Boxes function into Value. 
 */
RVal * fromFunction(::Function * from);

/** Returns a subset of given value. 
 */
RVal * genericGetElement(RVal * from, RVal * index);

/** Sets given subset of the value. 
 */
void genericSetElement(RVal * target, RVal * index, RVal * value);

/** Adds or concatenates two values. 
 */
RVal * genericAdd(RVal * lhs, RVal * rhs);

/** Subtracts two boxed double vectors, or raises an error. 
 */
RVal * genericSub(RVal * lhs, RVal * rhs);

/** Multiplies two boxed double vectors, or raises an error.
*/
RVal * genericMul(RVal * lhs, RVal * rhs);

/** Divides two boxed double vectors, or raises an error.
*/
RVal * genericDiv(RVal * lhs, RVal * rhs);

/** Compares the equality of two values. 
 */
RVal * genericEq(RVal * lhs, RVal * rhs);

/** Compares the inequality of two values. 
 */
RVal * genericNeq(RVal * lhs, RVal * rhs);

/** Compares two values, raising error if they are not double vectors. 
 */
RVal * genericLt(RVal * lhs, RVal * rhs);

/** Compares two values, raising error if they are not double vectors.
*/
RVal * genericGt(RVal * lhs, RVal * rhs);

/** Creates new function and binds it to the specified environment. 

The function is specified by its index (each function is assigned this index when it is compiled, so that the native code can be quickly recovered. 
 */
::Function * createFunction(int index, Environment * env);

/** Given any value, converts it to a single boolean result. 

The result of this call is used in conditional branches (if-then-else and while loop). 
 */
bool toBoolean(RVal * value);

/** Calls the given function with specified arguments. 

The number of arguments is matched agains the arguments of the function, new environment is created for the callee and its arguments are populated by the values given. Then the function is called, and its return value is returned. 

 */
RVal * call(RVal * callee, unsigned argc, ...);

/** Returns the length of given value if it is a vector. 
 */
double length(RVal * value);

/** Returns the type of given value. The result is a character vector of either 'double', 'character', or 'function'. 
 */
CharacterVector * type(RVal * value);

/** Evaluates given rift source in the specified environment and returns the return value. 
 */
RVal * eval(Environment * env, char const * value);

/** Evaluates given value, raising an error if it is not a characer vector, returning the result otherwise. */
RVal * genericEval(Environment * env, RVal * value);

/** Joins given values together, raising an error if they are not of the same type, or if there is a function among them. 
 */
RVal * c(int size, ...);

}


#endif // RUNTIME_H


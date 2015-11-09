#pragma once
#ifndef RUNTIME_H
#define RUNTIME_H

#include <ciso646>
#include "llvm.h"
#include "ast.h"

/** Value forward declaration 
*/
struct Value;

/** Character vector. 

Character vector is essentally a string. Any non-negative lengths are supported and characters are null terminated. The size of the character vector is the size of the data array minus one for the null termination which is not user accessible. 
*/
struct CharacterVector {
    char * data;

    unsigned size;

    /** Creates the character vector from given data and size. 
    
    Takes ownership of the data. 
    */
    CharacterVector(char * data, unsigned size):
        data(data),
        size(size) {
    }

    /** Creates the character vector from existing null terminated string. 
    
    Creates a copy of the string internally. 
    */
    CharacterVector(char const * from) {
        size = strlen(from);
        data = new char[size + 1];
        memcpy(data, from, size + 1);
    }

    /** Deletes the character vector and its data. 
    */
    ~CharacterVector() {
        delete [] data;
    }

    /** Prints the character vector to given stream. 
     */
    void print(std::ostream & s) {
        s << data;
    }

};

/** Double vector implementation. 

Contains array of double values together with its size. 
*/
struct DoubleVector {
    
    double * data;

    unsigned size;
    
    /** Creates double vector from given double scalar. 
     */
    DoubleVector(double value):
        data(new double[1]),
        size(1) {
        data[0] = value;
    }

    /** Creates double vector from given double array and its size. 
    
    The data array will become owned by the vector. 
    */
    DoubleVector(double * data, unsigned size):
        data(data),
        size(size) {
    }

    /** Creates double vector from given doubles. 
     */
    DoubleVector(std::initializer_list<double> d) {
        size = d.size();
        data = new double[size];
        unsigned i = 0;
        for (double dd : d)
            data[i++] = dd;
    }

    /** Deletes the vector and its data. 
     */
    ~DoubleVector() {
        delete [] data;
    }

    /** Prints the vector to given stream. 
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
    ::Value * value;
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
    ::Value * get(rift::Symbol symbol) {
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
    void set(rift::Symbol symbol, ::Value * value) {
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
typedef ::Value * (*FunPtr)(Environment *);

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
        ::Function * f;
    };

    /** Creates a boxed double vector from given numbers. 
     */
    Value(std::initializer_list<double> d) :
        type(Type::Double),
        d(new DoubleVector(d)) {
    }

    /** Creates a boxed character vector from given string. 
     */
    Value(char const * c) :
        type(Type::Character),
        c(new CharacterVector(c)) {
    }

    /** Boxes given DoubleVector into a Value. 

    Takes ownership of the vector. 
     */
    Value(DoubleVector * d):
        type(Type::Double),
        d(d) {
    }

    /** Boxes given CharacterVector into a Value.

    Takes ownership of the vector.
    */
    Value(CharacterVector * c):
        type(Type::Character),
        c(c) {
    }

    /** Boxes given Function into a Value.

    Takes ownership of the vector.
    */
    Value(::Function * f):
        type(Type::Function),
        f(f) {
    }

    /** Deletes the value, deleting its boxed element based on value's type. 
     */
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
    bool operator == (Value const & other) {
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
    bool operator != (Value const & other) {
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
inline std::ostream & operator << (std::ostream & s, ::Value const & value) {
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
::Value * envGet(Environment * env, int symbol);

/** Binds given symbol to the value in the specified environment. 
 */
void envSet(Environment * env, int symbol, ::Value * value);

/** Creates a double vector from the literal. 
 */
DoubleVector * doubleVectorLiteral(double value);

/** Creates a character vector from the literal. 
 */
CharacterVector * characterVectorLiteral(int cpIndex);

/** Boxes double vector into a Value. 
 */
::Value * fromDoubleVector(DoubleVector * from);

/** Boxes character vector into a Value. 
 */
::Value * fromCharacterVector(CharacterVector * from);

/** Boxes function into Value. 
 */
::Value * fromFunction(::Function * from);

/** Unboxes value to the double vector it contains. 
 */
DoubleVector * doubleFromValue(::Value * v);

/** Unboxes double vector to the scalar double it contains. 
 */
double scalarFromVector(DoubleVector * v);

/** Unboxes value to the character vector it contains. 
 */
CharacterVector * characterFromValue(::Value *v);

/** Unboxes value to the function object it contains. 
 */
::Function * functionFromValue(::Value *v);

/** Returns a scalar double element from given double vector. 
 */
double doubleGetSingleElement(DoubleVector * from, double index);

/** Returns a subset of double vector. 
 */
DoubleVector * doubleGetElement(DoubleVector * from, DoubleVector * index);

/** Returns a subset of character vector. 
 */
CharacterVector * characterGetElement(CharacterVector * from, DoubleVector * index);

/** Returns a subset of given value. 
 */
::Value * genericGetElement(::Value * from, ::Value * index);

/** Sets the index-th element of given double vector. 
 */
void doubleSetElement(DoubleVector * target, DoubleVector * index, DoubleVector * value);

/** Sets the specified subset of given double vector. 
 */
void scalarSetElement(DoubleVector * target, double index, double value);

/** Sets the given subset of character vector. 
 */
void characterSetElement(CharacterVector * target, DoubleVector * index, CharacterVector * value);

/** Sets given subset of the value. 
 */
void genericSetElement(::Value * target, ::Value * index, ::Value * value);

/** Adds two double vectors. 
 */
DoubleVector * doubleAdd(DoubleVector * lhs, DoubleVector * rhs);

/** Concatenates two character vectors. 
 */
CharacterVector * characterAdd(CharacterVector * lhs, CharacterVector * rhs);

/** Adds or concatenates two values. 
 */
::Value * genericAdd(::Value * lhs, ::Value * rhs);

/** Subtracts two double vectors. 
 */
DoubleVector * doubleSub(DoubleVector * lhs, DoubleVector * rhs);

/** Subtracts two boxed double vectors, or raises an error. 
 */
::Value * genericSub(::Value * lhs, ::Value * rhs);

/** Multiplies two double vectors. 
 */
DoubleVector * doubleMul(DoubleVector * lhs, DoubleVector * rhs);

/** Multiplies two boxed double vectors, or raises an error.
*/
::Value * genericMul(::Value * lhs, ::Value * rhs);

/** DIvides two double vectors. 
 */
DoubleVector * doubleDiv(DoubleVector * lhs, DoubleVector * rhs);

/** Divides two boxed double vectors, or raises an error.
*/
::Value * genericDiv(::Value * lhs, ::Value * rhs);

/** Compares the equality of two double vectors. 
 */
DoubleVector * doubleEq(DoubleVector * lhs, DoubleVector * rhs);

/** Compares the equality of two character vectors. 
 */
DoubleVector * characterEq(CharacterVector * lhs, CharacterVector * rhs);

/** Compares the equality of two values. 
 */
::Value * genericEq(::Value * lhs, ::Value * rhs);

/** Compaes the inequality of two double vectors. 
 */
DoubleVector * doubleNeq(DoubleVector * lhs, DoubleVector * rhs);

/** Compares the inequality of two character vectors. 
 */
DoubleVector * characterNeq(CharacterVector * lhs, CharacterVector * rhs);

/** Compares the inequality of two values. 
 */
::Value * genericNeq(::Value * lhs, ::Value * rhs);

/** Compares two double vectors. 
 */
DoubleVector * doubleLt(DoubleVector * lhs, DoubleVector * rhs);

/** Compares two values, raising error if they are not double vectors. 
 */
::Value * genericLt(::Value * lhs, ::Value * rhs);

/** Compares two double vectors.
*/
DoubleVector * doubleGt(DoubleVector * lhs, DoubleVector * rhs);

/** Compares two values, raising error if they are not double vectors.
*/
::Value * genericGt(::Value * lhs, ::Value * rhs);

/** Creates new function and binds it to the specified environment. 

The function is specified by its index (each function is assigned this index when it is compiled, so that the native code can be quickly recovered. 
 */
::Function * createFunction(int index, Environment * env);

/** Given any value, converts it to a single boolean result. 

The result of this call is used in conditional branches (if-then-else and while loop). 
 */
bool toBoolean(::Value * value);

/** Calls the given function with specified arguments. 

The number of arguments is matched agains the arguments of the function, new environment is created for the callee and its arguments are populated by the values given. Then the function is called, and its return value is returned. 

 */
::Value * call(::Value * callee, unsigned argc, ...);

/** Returns the length of given value if it is a vector. 
 */
double length(::Value * value);

/** Returns the type of given value. The result is a character vector of either 'double', 'character', or 'function'. 
 */
CharacterVector * type(::Value * value);

/** Evaluates given rift source in the specified environment and returns the return value. 
 */
::Value * eval(Environment * env, char const * value);

/** Evaluates given character vector in the specified environment and returns its result. 
 */
::Value * characterEval(Environment * env, CharacterVector * value);

/** Evaluates given value, raising an error if it is not a characer vector, returning the result otherwise. */
::Value * genericEval(Environment * env, ::Value * value);

/** Joins N double vectors together. 
 */
DoubleVector * doublec(int size, ...);

/** Joins N character vectors together. 
 */
CharacterVector * characterc(int size, ...);

/** Joins given values together, raising an error if they are not of the same type, or if there is a function among them. 
 */
::Value * c(int size, ...);

}


#endif // RUNTIME_H


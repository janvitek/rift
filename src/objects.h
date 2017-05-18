#pragma once

#include "gc.h"

template <typename T>
struct RValOps {
  public:
    static T* Cast(RVal* obj) {
        if (obj->type != T::TYPE)
            return nullptr;
        return (T*) obj;
    }

    RValOps<T>() = delete;
    RValOps<T>(RValOps<T> const&) = delete;

  protected:
    // Those operators are used internally by the different RVals to allocate a
    // new instance of themselves through the GC.
    //
    // The convention is that each RVal should implement at least one static
    // New method to create new instances.
    //
    // Constructors are disallowed because the type tag needs to be set by the
    // GC already. Otherwise a GC during object construction will result in an
    // untagged object being scanned. Since defining a constructor on a struct
    // might cause C++ to override the type tag we avoid using them.
    struct AllocPlain {
        T* operator() () const {
            return (T*) gc::GarbageCollector::alloc(sizeof(T), T::TYPE);
        }
    };
    struct AllocVect {
        T* operator() (unsigned length) const {
            return (T*) gc::GarbageCollector::alloc(
                    sizeof(T) + length * T::ELEMENT_SIZE, T::TYPE);
        }
    };
};

/* =================
 * Rift object types
 * =================
 */

/*
 *  Character vector
 *
 *  strings of variable size. They are null terminated. Size excludes the
 *  trailing terminator.
 *
 */
struct CharacterVector : RVal, RValOps<CharacterVector> {
    unsigned size;

    CharacterVector() = delete;

    static constexpr Type TYPE = Type::Character;
    static constexpr size_t ELEMENT_SIZE = sizeof(char);

    static CharacterVector* New(unsigned size) {
        CharacterVector* obj = AllocVect()(size+1);
        obj->size = size;
        obj->data[size] = 0;
        return obj;
    }

    static CharacterVector* New(const char* c) {
        size_t size = strlen(c);
        CharacterVector* obj = AllocVect()(size+1);
        obj->size = size;
        unsigned i = 0;
        while (*c && i < size) {
            (*obj)[i++] = *c++;
        }
        obj->data[size] = 0;
        return obj;
    }

    /** Prints to given stream. */
    void print(std::ostream & s) {
        s << data;
    }

    char& operator[] (const size_t i) {
        assert (i < size);
        return data[i];
    }

    char data[];
};

/*
 * A Double vector
 *
 * consists of an array of doubles and a size.
 * 
 */
struct DoubleVector : RVal, RValOps<DoubleVector> {
    unsigned size;

    static constexpr Type TYPE = Type::Double;
    static constexpr size_t ELEMENT_SIZE = sizeof(double);

    static DoubleVector* New(unsigned size) {
        DoubleVector* obj = AllocVect()(size);
        obj->size = size;
        return obj;
    }

    static DoubleVector* New(std::initializer_list<double> d) {
        DoubleVector* obj = AllocVect()(d.size());
        obj->size = d.size();
        unsigned i = 0;
        for (double dd : d)
            (*obj)[i++] = dd;
        return obj;
    }

    /** Prints to given stream. 
     */
    void print(std::ostream & s) {
        for (unsigned i = 0; i < size; ++i)
            s << data[i] << " ";
    }

    double& operator[] (const size_t i) {
        assert (i < size);
        return data[i];
    }

    double data[];
};

/*
 * Binding for the environment. 
 * List of pair of symbol and corresponding Value.
 *
 */
struct Bindings : RVal, RValOps<Bindings> {
    constexpr static size_t growSize = 4;
    constexpr static size_t initialSize = 4;

    struct Binding {
        rift::Symbol symbol;
        RVal * value;
    };

    unsigned size;
    unsigned available;
    
    static constexpr Type TYPE = Type::Bindings;
    static constexpr size_t ELEMENT_SIZE = sizeof(Binding);

    static Bindings* New(unsigned size = initialSize) {
        Bindings* obj = AllocVect()(size);
        obj->size = 0;
        obj->available = size;
        return obj;
    }

    /** Returns the value corresponding to given Symbol. 

    If the symbol is not found in current bindings, recursively searches parent environments as well. 
     */
    RVal * get(rift::Symbol symbol) {
        for (unsigned i = 0; i < size; ++i)
            if (binding[i].symbol == symbol)
                return binding[i].value;
        return nullptr;
    }

    /** Assigns given value to symbol. 
    
    If the symbol already exists it the environment, updates its value,
    otherwise creates new binding for the symbol and attaches it to the value. 
     */
    bool set(rift::Symbol symbol, RVal * value) {
        for (unsigned i = 0; i < size; ++i)
            if (binding[i].symbol == symbol) {
                binding[i].value = value;
                return true;
            }
        if (size < available) {
            binding[size].symbol = symbol;
            binding[size].value = value;
            size++;
            return true;
        }
        return false;
    }

    Bindings* grow() {
        Bindings* g = Bindings::New(available + growSize);
        memcpy(g->binding, binding, sizeof(Binding)*size);
        g->size = size;
        return g;
    }

    Binding binding[];
};

/*
 * Rift Environment. 
 * 
 * Environment represents function's local variables as well as a pointer to
 * the parent environment (i.e. the environment in which the function was
 * declared).
 *
 * The bindings are stored externally because we need to be able to grow
 * them dynamically.
 *
 */
struct Environment : RVal, RValOps<Environment> {
    /** Parent environment. 

    Nullptr if top environment. 
     */
    Environment * parent;
    Bindings * bindings;

    static constexpr Type TYPE = Type::Environment;

    static Environment* New(Environment* parent) {
        Environment* obj = AllocPlain()();
        obj->bindings = nullptr;
        obj->parent = parent;
        return obj;
    }

    static Environment* New(Environment* parent, Bindings* bindings) {
        Environment* obj = AllocPlain()();
        obj->bindings = bindings;
        obj->parent = parent;
        return obj;
    }

    RVal * get(rift::Symbol symbol) {
        if (bindings) {
            RVal* res = bindings->get(symbol);
            if (res)
                return res;
        }

        if (parent != nullptr)
            return parent->get(symbol);
        else
            throw "Variable not found";
    }

    /** Assigns given value to symbol. 

    If the symbol already exists it the environment, updates its value,
    otherwise creates new binding for the symbol and attaches it to the value. 
     */
    void set(rift::Symbol symbol, RVal * value) {
        if (!bindings)
            bindings = Bindings::New();

        if (bindings->set(symbol, value))
            return;

        bindings = bindings->grow();
        bool succ __attribute__((unused)) =
            bindings->set(symbol, value);
        assert(succ);
    }

};

/*
 * Pointer to Rift function. These functions all takes Environment*
 * and return an RVal*
 *
 */
typedef RVal * (*FunPtr)(Environment *);

/*
 * List of formal arguments. Stored externally to be able to share between
 * functions.
 *
 */
struct FunctionArgs : RVal, RValOps<FunctionArgs> {
    static constexpr Type TYPE = Type::FunctionArgs;
    static constexpr size_t ELEMENT_SIZE = sizeof(rift::Symbol);

    unsigned length;

    static FunctionArgs* New(std::vector<rift::ast::Var*>& args, unsigned length) {
        FunctionArgs* obj = AllocVect()(length);
        unsigned i = 0;
        obj->length = length;
        for (rift::ast::Var * arg : args)
            (*obj)[i++] = arg->symbol;
        return obj;
    }

    rift::Symbol& operator[] (const size_t i) {
        assert (i < length);
        return symbol[i];
    }

    rift::Symbol symbol[];
};

/*
 * Rift Function.
 *
 * Each function contains an environment, compiled code, bitcode, and the list
 * of formal parameters. The bitcode and argument names are there for debugging
 * purposes.
 *
 */
struct RFun : RVal, RValOps<RFun> {
    Environment * env;
    FunPtr code;
    llvm::Function * bitcode;
    FunctionArgs * args;
    
    static constexpr Type TYPE = Type::Function;

    static RFun* New(rift::ast::Fun * fun, llvm::Function * bitcode) {
        RFun* obj = AllocPlain()();
        obj->env = nullptr;
        obj->code = nullptr;
        obj->bitcode = bitcode;
        obj->args = nullptr;
        if (fun->args.size() > 0) {
            obj->args = FunctionArgs::New(fun->args, fun->args.size());
        }
        return obj;
    }

    static RFun* Copy(RFun* fun) {
        RFun* obj = AllocPlain()();
        obj->env = fun->env;
        obj->code = fun->code;
        obj->bitcode = fun->bitcode;
        obj->args = fun->args;
        return obj;
    }

    size_t nargs() {
        if (!args)
            return 0;
        return args->length;
    }
    /** Create a closure by copying function f and binding it to environment
	e. Arguments are shared.
     */
    RFun* close(Environment * e) {
        assert(!env);
        RFun* closure = Copy(this);
        closure->env = e;
        return closure;
    }
  
    /** Prints bitcode to given stream.  */
    void print(std::ostream & s) {
        llvm::raw_os_ostream ss(s);
        bitcode->print(ss);
    }

};

/** Prints to given stream.  */
void RVal::print(std::ostream & s) {
         if (auto d = DoubleVector::Cast(this))    d->print(s);
    else if (auto c = CharacterVector::Cast(this)) c->print(s);
    else if (auto f = RFun::Cast(this))            f->print(s);
    else assert(false);
}

/** Standard C++ printing support. */
inline std::ostream & operator << (std::ostream & s, RVal & value) {
    value.print(s);
    return s;
}

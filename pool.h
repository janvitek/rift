#pragma once
#ifndef POOL_H
#define POOL_H

#include "runtime.h"

namespace rift {

/** The constant pool holds symbols, strings and all compiled functions.*/
class Pool {

public:
    /** Returns the number of compiled functions.  */
    static unsigned functionsCount() {
        return f_.size();
    }

    /** Returns function at index.   */
    static RFun * getFunction(int index) {
        return f_[index];
    }

    /** Adds function to compiled functions, returns its index.     */
    static int addFunction(ast::Fun * fun, llvm::Function * bitcode) {
        RFun * f = new RFun(fun, bitcode);
        f_.push_back(f);
        return f_.size() - 1;
    }

    /** Returns string at index.  */
    static std::string const & getPoolObject(unsigned index) {
        return pool_[index];
    }

    /** Adds string to the constant pool.  */
    static int addToPool(std::string const & s) {
        for (unsigned i = 0; i < pool_.size(); ++i)
            if (pool_[i] == s)
                return i;
        pool_.push_back(s);
        return pool_.size() - 1;
    }

private:
    /** Compiled functions.   */
    static std::vector<RFun *> f_;

    /** Strings.  */
    static std::vector<std::string> pool_;
};

}
#endif // POOL_H


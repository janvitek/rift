#pragma once
#ifndef POOL_H
#define POOL_H

#include "runtime.h"

namespace rift {

/** Runtime static class. 

Contains the list of constant pool objects (symbols and character vectors) and the list of all compiled functions. 
*/
class Pool {
public:
    /** Returns the number of already stored compiled functions. 
     */
    static unsigned functionsCount() {
        return f_.size();
    }

    /** Returns the index-th compiled function. 
     */
    static RFun * getFunction(int index) {
        return f_[index];
    }

    /** Adds given function to the list of compiled objects. 
     */
    static int addFunction(ast::Fun * fun, llvm::Function * bitcode) {
        RFun * f = new RFun(fun, bitcode);
        f_.push_back(f);
        return f_.size() - 1;
    }

    /** Returns the index-th constant pool object. 
     */
    static std::string const & getPoolObject(unsigned index) {
        return pool_[index];
    }

    /** Adds given string to the constant pool list. 
     */
    static int addToPool(std::string const & s) {
        for (unsigned i = 0; i < pool_.size(); ++i)
            if (pool_[i] == s)
                return i;
        pool_.push_back(s);
        return pool_.size() - 1;
    }

private:

    /** List of compiled functions. 
     */
    static std::vector<RFun *> f_;

    /** List of constant pool objects. 
     */
    static std::vector<std::string> pool_;
};


}

#endif // POOL_H


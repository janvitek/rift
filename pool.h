#pragma once
#ifndef POOL_H
#define POOL_H

#include <string>
#include <vector>

namespace rift {

/** Runtime static class. 

Contains the list of constant pool objects (symbols and character vectors) and the list of all compiled functions. 
*/
class Pool {
public:
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

    /** List of constant pool objects. 
     */
    static std::vector<std::string> pool_;
};


}

#endif // POOL_H


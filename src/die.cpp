#if VERSION > 10

#include <iostream>

#include "die.h"
#include "rift.h"

using namespace llvm;

namespace rift {
char DeadInstructionElimination::ID = 0;

bool DeadInstructionElimination::runOnFunction(llvm::Function & f) {
    bool changed = false;
    while (true) {
        bool c = false;
        for (auto & b : f) {
            auto i = b.begin();
            while (i != b.end()) {
                bool erase = false;
                if (CallInst * ci = dyn_cast<CallInst>(i)) {
                    if (ci->getCalledFunction()->getAttributes().hasAttribute(AttributeSet::FunctionIndex, Attribute::ReadNone)) {
                        if (ci->use_empty())
                            erase = true;
                    }
                }
                if (erase) {
                    llvm::Instruction * v = &*i;
                    ++i;
                    v->eraseFromParent();
                    c = true;
                } else {
                    ++i;
                }
            }
        }
        changed = changed or c;
        if (not c)
            break;
    }
    if (DEBUG) {
        cout << "After die removal: ---------------------------------------" << endl;
        f.dump();
    }
    return changed;
}

} // namespace rift

#endif //VERSION
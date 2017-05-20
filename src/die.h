#if VERSION > 10
#pragma once

#include "llvm.h"
#include "specialize.h"

namespace rift {

/** Dead Instruction Elimination removes all pure (readnone in llvm's terminology) functions and all other instructions whose result is never used. This gets rid of the generic cases that were replaced in the specialization, but not actually deleted. 
 */
class DeadInstructionElimination : public llvm::FunctionPass {
public:
    static char ID;
    DeadInstructionElimination() : llvm::FunctionPass(ID) {}

    llvm::StringRef getPassName() const override { return "DeadInstructionElimination"; }

    bool runOnFunction(llvm::Function & f) override;
};
} // namespace rift

#endif //VERSION
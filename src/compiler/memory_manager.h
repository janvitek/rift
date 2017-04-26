#pragma once
#include "llvm.h"
#include "runtime.h"
#include "specializedRuntime.h"

namespace rift {

/** The Rift Memory manager extends the default LLVM memory manager with
    support for resolving the Rift runtime functions. This is achieved by
    extending the behavior of the getSymbolAddress function.
  */

class MemoryManager : public llvm::SectionMemoryManager {

public:
#define NAME_IS(name) if (Name == #name) return reinterpret_cast<uint64_t>(::name)
    /** Return the address of symbol, or nullptr if undefind. We extend the
        default LLVM resolution with the list of RIFT runtime functions.
      */
    uint64_t getSymbolAddress(const std::string & Name) override {
        uint64_t addr = SectionMemoryManager::getSymbolAddress(Name);
        if (addr != 0) return addr;
        // This bit is for some OSes (Windows and OSX where the MCJIT symbol
        // loading is broken)
        NAME_IS(envCreate);
        NAME_IS(envGet);
        NAME_IS(envSet);
        NAME_IS(doubleVectorLiteral);
        NAME_IS(characterVectorLiteral);
        NAME_IS(scalarFromVector);
        NAME_IS(doubleGetSingleElement);
        NAME_IS(doubleGetElement);
        NAME_IS(characterGetElement);
        NAME_IS(genericGetElement);
        NAME_IS(doubleSetElement);
        NAME_IS(scalarSetElement);
        NAME_IS(characterSetElement);
        NAME_IS(genericSetElement);
        NAME_IS(doubleAdd);
        NAME_IS(characterAdd);
        NAME_IS(genericAdd);
        NAME_IS(doubleSub);
        NAME_IS(genericSub);
        NAME_IS(doubleMul);
        NAME_IS(genericMul);
        NAME_IS(doubleDiv);
        NAME_IS(genericDiv);
        NAME_IS(doubleEq);
        NAME_IS(characterEq);
        NAME_IS(genericEq);
        NAME_IS(doubleNeq);
        NAME_IS(characterNeq);
        NAME_IS(genericNeq);
        NAME_IS(doubleLt);
        NAME_IS(genericLt);
        NAME_IS(doubleGt);
        NAME_IS(genericGt);
        NAME_IS(createFunction);
        NAME_IS(toBoolean);
        NAME_IS(call);
        NAME_IS(length);
        NAME_IS(type);
        NAME_IS(eval);
        NAME_IS(characterEval);
        NAME_IS(genericEval);
        NAME_IS(doublec);
        NAME_IS(characterc);
        NAME_IS(c);
        llvm::report_fatal_error("Extern function '" + Name + "' couldn't be resolved!");
    }
};


}

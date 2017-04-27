#pragma once
#include "llvm.h"
#include "runtime.h"
#include "pool.h"
#include "compiler.h"
#include "types.h"
#include "module.h"
#include "memory_manager.h"

#include "rift.h"

#include "type_checker.h"
#include "type_analysis.h"
#include "unboxing.h"
#include "specialize.h"
#include "boxing_removal.h"


namespace rift {

class JIT {
public:
    /** Compiles a function and returns a pointer to the native code.  JIT
      compilation in LLVM finalizes the module, this function can only be
      called once.
      */
    static FunPtr compile(ast::Fun * f) {
        Compiler c;
        unsigned start = Pool::functionsCount();
        int result = c.compile(f);
        llvm::ExecutionEngine * engine =
            llvm::EngineBuilder(std::unique_ptr<llvm::Module>(c.m.get()))
                .setMCJITMemoryManager(
                        std::unique_ptr<MemoryManager>(new MemoryManager()))
                .create();
        optimizeModule(engine, c.m.get());
        engine->finalizeObject();
        // Compile newly registered functions; update their native code in the
        // registered functions vector
        for (; start < Pool::functionsCount(); ++start) {
            RFun * rec = Pool::getFunction(start);
            rec->code = reinterpret_cast<FunPtr>(engine->getPointerToFunction(rec->bitcode));
        }
        return Pool::getFunction(result)->code;
    }

private:

    static void optimize() {
//        llvm::FunctionPassManager
    }

/*
    void InitializeModuleAndPassManager(void) {
      // Open a new module.
      TheModule = llvm::make_unique<Module>("my cool jit", TheContext);

      // Create a new pass manager attached to it.
      TheFPM = llvm::make_unique<FunctionPassManager>(TheModule.get());

      // Do simple "peephole" optimizations and bit-twiddling optzns.
      TheFPM->add(createInstructionCombiningPass());
      // Reassociate expressions.
      TheFPM->add(createReassociatePass());
      // Eliminate Common SubExpressions.
      TheFPM->add(createGVNPass());
      // Simplify the control flow graph (deleting unreachable blocks, etc).
      TheFPM->add(createCFGSimplificationPass());

      TheFPM->doInitialization();
    }     */


    /** Optimize on the bitcode before native code generation. The
      TypeAnalysis, Unboxing and BoxingRemoval are Rift passes, the rest is
      from LLVM.
      */
    static void optimizeModule(llvm::ExecutionEngine * ee, llvm::Module * m) {
        auto *pm = new llvm::legacy::FunctionPassManager(m);
        m->setDataLayout(ee->getDataLayout());
        pm->add(new TypeChecker());
        pm->add(new TypeAnalysis());
        pm->add(new Unboxing());
        pm->add(new Specialize());
        pm->add(new BoxingRemoval());
        pm->add(llvm::createConstantPropagationPass());
        // Optimize each function of this module
        for (llvm::Function & f : *m) {
            if (not f.empty()) {
                if (DEBUG) {
                    std::cout << "After translation to bitcode: -------------------------------" << std::endl;
                    f.dump();
                }
                pm->run(f);
                if (DEBUG) {
                    std::cout << "After LLVM's constant propagation: --------------------------" << std::endl;
                    f.dump();
                }
            }
        }
        delete pm;
    }

};



}

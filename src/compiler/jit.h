#pragma once
#include "rift.h"
#include "llvm.h"
#include "runtime.h"
#include "pool.h"
#include "compiler.h"
#include "types.h"
#include "runtime.h"
#include "rift.h"

#if VERSION > 10
#include "specializedRuntime.h"

#include "type_checker.h"
#include "type_analysis.h"
#include "unboxing.h"
#include "specialize.h"
#include "boxing_removal.h"
#endif //VERSION

namespace rift {

class JIT {

private:
    typedef llvm::orc::ObjectLinkingLayer<> ObjectLayer;
    typedef llvm::orc::IRCompileLayer<ObjectLayer> CompileLayer;
    typedef std::function<std::unique_ptr<llvm::Module>(std::unique_ptr<llvm::Module>)> OptimizeFunction;
    typedef llvm::orc::IRTransformLayer<CompileLayer, OptimizeFunction> OptimizeLayer;
    typedef llvm::orc::CompileOnDemandLayer<OptimizeLayer> CompileOnDemandLayer;

public:
    typedef CompileOnDemandLayer::ModuleSetHandleT ModuleHandle;

    /** Compiles a function and returns a pointer to the native code.  JIT
      compilation in LLVM finalizes the module, this function can only be
      called once.
      */
    static FunPtr compile(ast::Fun * f) {
        Compiler c;
        unsigned start = Pool::functionsCount();
        int result = c.compile(f);
        llvm::Module * m = c.m.release();
        lastModule_ = singleton().addModule(std::unique_ptr<llvm::Module>(m));
        lastModuleDeletable_ = Pool::functionsCount() == start + 1;

        for (; start < Pool::functionsCount(); ++start) {
            RFun * rec = Pool::getFunction(start);
            rec->code = reinterpret_cast<FunPtr>(singleton().findSymbol(STR(start)).getAddress());
        }
        return Pool::getFunction(result)->code;
    }

    static void removeLastModule() {
        // TODO: llvm bug: removing a module corrupts the eh section and
        // subsequent C++ exception crash while unwinding the stack. Therefore
        // we disable deleting modules for now.
        if (false && lastModuleDeletable_) {
            singleton().removeModule(lastModule_);
        }
    }

    JIT():
        targetMachine(llvm::EngineBuilder().selectTarget()),
        dataLayout(targetMachine->createDataLayout()),
        compileLayer(objectLayer, llvm::orc::SimpleCompiler(*targetMachine)),
        optimizeLayer(compileLayer,
            [this](std::unique_ptr<llvm::Module> m) {
                optimizeModule(m.get());
                return m;
            }
        )
        ,compileCallbackManager(
             llvm::orc::createLocalCompileCallbackManager(targetMachine->getTargetTriple(), 0)
        ),
        codLayer(optimizeLayer,
            [this](llvm::Function & f) {
                return std::set<llvm::Function*>({&f});
            },
            *compileCallbackManager,
            llvm::orc::createLocalIndirectStubsManagerBuilder(
                targetMachine->getTargetTriple()))
        {
            // this loads the host process itself, making
            // the symbols in it available for execution
            llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr);
        }

    ModuleHandle addModule(std::unique_ptr<llvm::Module> m) {
        auto resolver = llvm::orc::createLambdaResolver(
            // the first lambda looks in symbols in the JIT itself
            [this](std::string const & name) {
                if (auto s = this->codLayer.findSymbol(name, false))
                    return s;
                return llvm::JITSymbol(nullptr);
            },

            // the second lamba looks for symbols in the host process
            [](std::string const & name) {
                if (auto symAddr = llvm::RTDyldMemoryManager::getSymbolAddressInProcess(name))
                    return llvm::JITSymbol(symAddr, llvm::JITSymbolFlags::Exported);
                return JIT::fallbackHostSymbolResolver(name);
          });

        m->setDataLayout(dataLayout);

        // Build a singleton module set to hold our module.
        std::vector<std::unique_ptr<llvm::Module>> ms;
        ms.push_back(std::move(m));

        // Add the set to the JIT with the resolver we created above and a newly
        // created SectionMemoryManager.
        return codLayer.addModuleSet(std::move(ms),
            llvm::make_unique<llvm::SectionMemoryManager>(),
            std::move(resolver));
    }

    llvm::JITSymbol findSymbol(const std::string name) {
        std::string mangled;
        llvm::raw_string_ostream mangledStream(mangled);
        llvm::Mangler::getNameWithPrefix(mangledStream, name, dataLayout);
        return codLayer.findSymbol(mangledStream.str(), true);
    }

    void removeModule(ModuleHandle m) {
        codLayer.removeModuleSet(m);
    }


private:

    static ModuleHandle lastModule_;
    static bool lastModuleDeletable_;

    static llvm::JITSymbol fallbackHostSymbolResolver(std::string const & name) {

#define FUN_PURE(NAME, ...) if (name == #NAME) \
    return llvm::JITSymbol(reinterpret_cast<uint64_t>(::NAME), llvm::JITSymbolFlags::Exported);
#define FUN(NAME, ...) if (name == #NAME) \
    return llvm::JITSymbol(reinterpret_cast<uint64_t>(::NAME), llvm::JITSymbolFlags::Exported);
RUNTIME_FUNCTIONS
#undef FUN_PURE
#undef FUN
        return llvm::JITSymbol(nullptr);
    }

    /** Optimize on the bitcode before native code generation. The
      TypeAnalysis, Unboxing and BoxingRemoval are Rift passes, the rest is
      from LLVM.
      */
    static void optimizeModule(llvm::Module * m) {
#if VERSION > 10
        auto pm = std::unique_ptr<llvm::legacy::FunctionPassManager>(new llvm::legacy::FunctionPassManager(m));
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
#endif //VERSION
    }

    static JIT & singleton();
    std::unique_ptr<llvm::TargetMachine> targetMachine;
    const llvm::DataLayout dataLayout;
    ObjectLayer objectLayer;
    CompileLayer compileLayer;
    OptimizeLayer optimizeLayer;
    std::unique_ptr<llvm::orc::JITCompileCallbackManager> compileCallbackManager;
    CompileOnDemandLayer codLayer;
};

}

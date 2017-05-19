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

/** A just in time compiler using LLVM. The main entry point is
 the compile() static function which takes a reference to an AST
 node representing a function definition, and returns a pointer.
 
 This use the LLVM ORC JIT interface.
 
 */
class JIT {
private:
    // shorter names...
    typedef llvm::orc::ObjectLinkingLayer<> ObjectLayer;
    typedef llvm::orc::IRCompileLayer<ObjectLayer> CompileLayer;
    typedef function<unique_ptr<llvm::Module>(unique_ptr<llvm::Module>)> OptimizeModule;
    typedef llvm::orc::IRTransformLayer<CompileLayer, OptimizeModule> OptimizeLayer;
    typedef llvm::orc::CompileOnDemandLayer<OptimizeLayer> CompileOnDemandLayer;
public:
    /** Externally, the JIT uses this type to refer to a module that has been passed to it. 
     */
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
        lastModule_ = singleton().addModule(unique_ptr<llvm::Module>(m));
        lastModuleDeletable_ = Pool::functionsCount() == start + 1;

        for (; start < Pool::functionsCount(); ++start) {
            RFun * rec = Pool::getFunction(start);
            rec->code = reinterpret_cast<FunPtr>(singleton().findSymbol(STR(start)).getAddress());
        }
        return Pool::getFunction(result)->code;
    }

    /** If the last module is no longer needed, i.e. if it only contains a top-level expression, deletes the module and frees up the resources. 

    Does nothing if the last module also defines functions that are still callable. 
     */
    static void removeLastModule() {
        // TODO: llvm bug: removing a module corrupts the eh section and
        // subsequent C++ exception crash while unwinding the stack. Therefore
        // we disable deleting modules for now.
        if (false && lastModuleDeletable_) {
            singleton().removeModule(lastModule_);
        }
    }

private:

    /* Last module handle so that we can delete it later */
    static ModuleHandle lastModule_;
    /* Determines whether the last added module can be deleted after it executes. 
     */
    static bool lastModuleDeletable_;

    /* Creates the jit object and initializes the JIT layers. 
     */
    JIT():
        arch(llvm::EngineBuilder().selectTarget()),
        layout(arch->createDataLayout()),
        compiler(linker, llvm::orc::SimpleCompiler(*arch)),
        optimizer(compiler,
            [this](unique_ptr<llvm::Module> m) {
                optimizeModule(m.get());
                return m;
            }
        )
        ,callbackManager(
             llvm::orc::createLocalCompileCallbackManager(arch->getTargetTriple(), 0)
        ),
        cod(optimizer,
            [this](llvm::Function & f) {
                return set<llvm::Function*>({&f});
            },
            *callbackManager,
            llvm::orc::createLocalIndirectStubsManagerBuilder(
                arch->getTargetTriple()))
        {
            // this loads the host process itself, making
            // the symbols in it available for execution
            llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr);
    }

    /* Submits the llvm module to the JIT. 

    The module is consumed by the JIT and ModuleHandle is reuturned for future reference. 
     */
    ModuleHandle addModule(unique_ptr<llvm::Module> m) {
        auto resolver = llvm::orc::createLambdaResolver(
            // the first lambda looks in symbols in the JIT itself
            [this](string const & name) {
                if (auto s = this->cod.findSymbol(name, false))
                    return s;
                return llvm::JITSymbol(nullptr);
            },

            // the second lamba looks for symbols in the host process
            [](string const & name) {
                if (auto symAddr = llvm::RTDyldMemoryManager::getSymbolAddressInProcess(name))
                    return llvm::JITSymbol(symAddr, llvm::JITSymbolFlags::Exported);
                return JIT::fallbackHostSymbolResolver(name);
          });

        m->setDataLayout(layout);

        // Build a singleton module set to hold our module.
        vector<unique_ptr<llvm::Module>> ms;
        ms.push_back(move(m));

        // Add the set to the JIT with the resolver we created above and a newly
        // created SectionMemoryManager.
        return cod.addModuleSet(move(ms),
            llvm::make_unique<llvm::SectionMemoryManager>(),
            move(resolver));
    }

    /* Finds the symbol in the dynamically linked code. 
     */
    llvm::JITSymbol findSymbol(const string name) {
        string mangled;
        llvm::raw_string_ostream mangledStream(mangled);
        llvm::Mangler::getNameWithPrefix(mangledStream, name, layout);
        return cod.findSymbol(mangledStream.str(), true);
    }

    /* Removes the module from memory and releases its resources. 
     */
    void removeModule(ModuleHandle m) {
        cod.removeModuleSet(m);
    }

    /* Resolves calls to runtime functions to their C functions since these are not part of the dynamically linked space, but are needed by the code. 
     */
    static llvm::JITSymbol fallbackHostSymbolResolver(string const & name) {

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
        auto pm = unique_ptr<llvm::legacy::FunctionPassManager>
                           (new llvm::legacy::FunctionPassManager(m));
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
                    cout << "After translation to bitcode: -------------------------------" << endl;
                    f.dump();
                }
                pm->run(f);
                if (DEBUG) {
                    cout << "After LLVM's constant propagation: --------------------------" << endl;
                    f.dump();
                }
            }
        }
#endif //VERSION
    }

    /** Return the one and only JIT object */
    static JIT & singleton();

    // arch holds the platform for which we generate code
    unique_ptr<llvm::TargetMachine> arch;
    // arch specific data layout
    const llvm::DataLayout layout;
    // the dynamic linker
    ObjectLayer linker;
    // the dynamic compiler from LLVMIR to arch
    CompileLayer compiler;
    // an optimizer from LLVMIR to LLVMIR
    OptimizeLayer optimizer;
    // produces stub functions
    unique_ptr<llvm::orc::JITCompileCallbackManager> callbackManager;
    /* called by stub functions, extracts the function from its module into a new one and submits it to the lower layers
     */
    CompileOnDemandLayer cod;
};

}

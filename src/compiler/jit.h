#pragma once
#include "llvm.h"
#include "runtime.h"
#include "pool.h"
#include "compiler.h"
#include "types.h"
#include "module.h"
#include "runtime.h"
#include "specializedRuntime.h"

#include "rift.h"

#include "type_checker.h"
#include "type_analysis.h"
#include "unboxing.h"
#include "specialize.h"
#include "boxing_removal.h"


namespace rift {

class JIT {
private:

    typedef llvm::orc::ObjectLinkingLayer<> ObjectLayer;
    typedef llvm::orc::IRCompileLayer<ObjectLayer> CompileLayer;

public:

    typedef CompileLayer::ModuleSetHandleT ModuleHandle;


    /** Compiles a function and returns a pointer to the native code.  JIT
      compilation in LLVM finalizes the module, this function can only be
      called once.
      */
    static FunPtr compile(ast::Fun * f) {
        Compiler c;
        unsigned start = Pool::functionsCount();
        int result = c.compile(f);

        optimizeModule(c.m.get());

        llvm::Module * m = c.m.release();
        singleton().addModule(std::unique_ptr<llvm::Module>(m));

        for (; start < Pool::functionsCount(); ++start) {
            RFun * rec = Pool::getFunction(start);
            rec->code = reinterpret_cast<FunPtr>(singleton().findSymbol(STR(start)).getAddress());
        }
        return Pool::getFunction(result)->code;
    }

    JIT():
        targetMachine(llvm::EngineBuilder().selectTarget()),
        dataLayout(targetMachine->createDataLayout()),
        compileLayer(objectLayer, llvm::orc::SimpleCompiler(*targetMachine)) {
        // this loads the host process itself, making the symbols in it available for execution
        llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr);
    }

    ModuleHandle addModule(std::unique_ptr<llvm::Module> m) {
        auto resolver = llvm::orc::createLambdaResolver(
            // the first lambda looks in symbols in the JIT itself
            [&](std::string const & name) {
                if (auto s = compileLayer.findSymbol(name, false))
                    return s;
                return llvm::JITSymbol(nullptr);
            },

            // the second lamba looks for symbols in the host process
            [](std::string const & name) {
                if (auto symAddr = llvm::RTDyldMemoryManager::getSymbolAddressInProcess(name))
                    return llvm::JITSymbol(symAddr, llvm::JITSymbolFlags::Exported);
                return JIT::fallbackHostSymbolResolver(name);
          });

        // Build a singleton module set to hold our module.
        std::vector<std::unique_ptr<llvm::Module>> ms;
        ms.push_back(std::move(m));

        // Add the set to the JIT with the resolver we created above and a newly
        // created SectionMemoryManager.
        return compileLayer.addModuleSet(std::move(ms),
            llvm::make_unique<llvm::SectionMemoryManager>(),
            std::move(resolver));
    }

    llvm::JITSymbol findSymbol(const std::string name) {
        std::string mangled;
        llvm::raw_string_ostream mangledStream(mangled);
        llvm::Mangler::getNameWithPrefix(mangledStream, name, dataLayout);
        return compileLayer.findSymbol(mangledStream.str(), true);
    }

    void removeModule(ModuleHandle m) {
        compileLayer.removeModuleSet(m);
    }


private:

    static llvm::JITSymbol fallbackHostSymbolResolver(std::string const & name) {
#define NAME_IS(NAME) if (name == #NAME) return llvm::JITSymbol(reinterpret_cast<uint64_t>(::NAME), llvm::JITSymbolFlags::Exported)
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
#undef NAME_IS
        return llvm::JITSymbol(nullptr);
    }

    /** Optimize on the bitcode before native code generation. The
      TypeAnalysis, Unboxing and BoxingRemoval are Rift passes, the rest is
      from LLVM.
      */
    static void optimizeModule(llvm::Module * m) {
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
    }

    static JIT & singleton();

    std::unique_ptr<llvm::TargetMachine> targetMachine;
    const llvm::DataLayout dataLayout;
    ObjectLayer objectLayer;
    CompileLayer compileLayer;


};



}

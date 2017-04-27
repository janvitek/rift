#include "module.h"
#include "compiler.h"


namespace rift {

RiftModule::RiftModule() :
    llvm::Module("rift", Compiler::context()) {
}

llvm::Function * RiftModule::declareFunction(char const * name, llvm::FunctionType * signature, llvm::Module * m) {
    return llvm::Function::Create(signature, llvm::Function::ExternalLinkage, name, m);
}


llvm::Function * RiftModule::declarePureFunction(char const * name, llvm::FunctionType * signature, llvm::Module * m) {
    llvm::Function * f = declareFunction(name, signature, m);
    llvm::AttributeSet as;
    llvm::AttrBuilder b;
    b.addAttribute(llvm::Attribute::ReadNone);
    as = llvm::AttributeSet::get(Compiler::context(),llvm::AttributeSet::FunctionIndex, b);
    f->setAttributes(as);
    return f;
}


}

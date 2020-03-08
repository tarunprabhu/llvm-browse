#include "Function.h"
#include "Argument.h"
#include "BasicBlock.h"
#include "DIUtils.h"
#include "Module.h"

#include <llvm/IR/Comdat.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/Support/Casting.h>

using llvm::cast;
using llvm::dyn_cast;
using llvm::isa;

namespace lb {

Function::Function(llvm::Function& llvm_f, Module& module) :
    Value(EntityKind::Function),
    INavigable(EntityKind::Function), IWrapper<llvm::Function>(llvm_f, module), comdat(nullptr),
    di(llvm_f.getSubprogram()) {
  set_tag(llvm_f.getName(), "@");
  for(llvm::Argument& arg : llvm_f.args())
    args.push_back(&get_module().add(arg, *this));
  for(llvm::BasicBlock& bb : llvm_f)
    bbs.push_back(&get_module().add(bb, *this));
  if(llvm::Comdat* llvm_c = llvm_f.getComdat())
    comdat = &module.get(*llvm_c);
  if(di) {
    source_name = DebugInfo::get_name(di);
    full_name = DebugInfo::get_full_name(di);
    set_source_defn(SourceRange(di->getFilename().data(), di->getLine(), 1));
  }
}

bool Function::has_source_info() const {
  return di;
}

bool Function::has_source_name() const {
  return get_source_name().size();
}

bool Function::has_full_name() const {
  return get_full_name().size();
}

llvm::StringRef Function::get_source_name() const {
  return llvm::StringRef(source_name);
}

llvm::StringRef Function::get_full_name() const {
  return llvm::StringRef(full_name);
}

llvm::StringRef Function::get_llvm_name() const {
  return get_llvm().getName();
}

const Comdat* Function::get_comdat() const {
  return comdat;
}

bool Function::is_mangled() const {
  if(has_source_name())
    return get_source_name().size() != get_llvm_name().size();
  return false;
}

bool Function::is_artificial() const {
  return has_source_info() ? di->isArtificial() : false;
}

bool Function::is_defined() const {
  return has_source_info() ? di->isDefinition() : false;
}

bool Function::is_method() const {
  return has_source_info() ? di->getContainingType() : false;
}

bool Function::is_virtual() const {
  if(has_source_info())
    return di->getVirtuality() & llvm::DISubprogram::SPFlagVirtual;
  return false;
}

bool Function::is_pure_virtual() const {
  if(has_source_info())
    return di->getVirtuality() & llvm::DISubprogram::SPFlagPureVirtual;
  return false;
}

bool Function::is_private() const {
  return has_source_info() ? di->isPrivate() : false;
}

bool Function::is_protected() const {
  return has_source_info() ? di->isProtected() : false;
}

bool Function::is_public() const {
  return has_source_info() ? di->isPublic() : false;
}

Argument& Function::get_arg(unsigned i) {
  return *args.at(i);
}

const Argument& Function::get_arg(unsigned i) const {
  return *args.at(i);
}

Function::Iterator
Function::begin() const {
  return bbs.cbegin();
}

Function::Iterator
Function::end() const {
  return bbs.cend();
}

llvm::iterator_range<Function::Iterator>
Function::blocks() const {
  return llvm::iterator_range<Function::Iterator>(bbs);
}

Function::ArgIterator
Function::arg_begin() const {
  return args.cbegin();
}

Function::ArgIterator
Function::arg_end() const {
  return args.cend();
}

llvm::iterator_range<Function::ArgIterator>
Function::arguments() const {
  return llvm::iterator_range<Function::ArgIterator>(args);
}

} // namespace lb

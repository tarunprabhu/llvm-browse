#include "Argument.h"
#include "DIUtils.h"
#include "Function.h"
#include "Module.h"

#include <llvm/Support/Casting.h>

using llvm::cast;
using llvm::dyn_cast;
using llvm::isa;

namespace lb {

Argument::Argument(const llvm::Argument& arg, Function& f, Module& module) :
    Value(EntityKind::Argument),
    INavigable(EntityKind::Argument),
    IWrapper<llvm::Argument>(arg, module),
    parent(f),
    di(nullptr) {
  ;
}

void
Argument::set_debug_info_node(const llvm::DILocalVariable* di) {
  this->di = di;
  if(di) {
  	source_name = DebugInfo::get_name(di);
  }
}

bool
Argument::has_source_info() const {
  return di;
}

bool
Argument::has_source_name() const {
  return get_source_name().size();
}

llvm::StringRef
Argument::get_source_name() const {
  return source_name;
}

llvm::StringRef
Argument::get_llvm_name() const {
  return get_tag();
}

bool
Argument::is_artificial() const {
  return has_source_info() ? di->isArtificial() : false;
}

Function&
Argument::get_function() {
  return parent;
}

const Function&
Argument::get_function() const {
  return parent;
}

Argument&
Argument::make(const llvm::Argument& llvm_a, Function& f, Module& module) {
  auto* arg = new Argument(llvm_a, f, module);
  f.m_args.emplace_back(arg);
  module.vmap[&llvm_a] = arg;
  
  return *arg;
}

} // namespace lb

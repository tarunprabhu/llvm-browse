#include "GlobalVariable.h"
#include "DIUtils.h"
#include "Logging.h"
#include "Module.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/Comdat.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>

using llvm::cast;
using llvm::dyn_cast;
using llvm::isa;

namespace lb {

GlobalVariable::GlobalVariable(const llvm::GlobalVariable& llvm_g,
                               Module& module) :
    Value(EntityKind::GlobalVariable),
    INavigable(EntityKind::GlobalVariable),
    IWrapper<llvm::GlobalVariable>(llvm_g, module),
    comdat(nullptr),
    di(nullptr) {
  if(llvm_g.hasName())
    set_tag(llvm_g.getName(), "@");
  else
    critical() << "Cannot set tag for unnamed global" << llvm_g << "\n";
  if(const llvm::Comdat* llvm_c = llvm_g.getComdat())
    comdat = &(static_cast<const Module&>(module).get(*llvm_c));

  llvm::SmallVector<llvm::DIGlobalVariableExpression*, 4> dis;
  llvm_g.getDebugInfo(dis);
  if(dis.size() == 1) {
    di = dis[0]->getVariable();
    set_source_defn(
        SourceRange(module.get_full_path(di->getDirectory(), di->getFilename()),
                    di->getLine(),
                    1));
    source_name    = DebugInfo::get_name(di);
    full_name      = DebugInfo::get_full_name(di);
    qualified_name = DebugInfo::get_qualified_name(di);
  } else if(dis.size() > 1) {
    warning() << "Could not find unique debug info for global: " << llvm_g
              << "\n";
  }
}

bool
GlobalVariable::has_source_info() const {
  return di;
}

bool
GlobalVariable::has_source_name() const {
  return get_source_name().size();
}

bool
GlobalVariable::has_full_name() const {
  return get_full_name().size();
}

bool
GlobalVariable::has_qualified_name() const {
	return get_qualified_name().size();
}

llvm::StringRef
GlobalVariable::get_source_name() const {
  return llvm::StringRef(source_name);
}

llvm::StringRef
GlobalVariable::get_llvm_name() const {
  return get_llvm().getName();
}

llvm::StringRef
GlobalVariable::get_qualified_name() const {
	return llvm::StringRef(qualified_name);
}

llvm::StringRef
GlobalVariable::get_full_name() const {
  return llvm::StringRef(full_name);
}

const Comdat*
GlobalVariable::get_comdat() const {
  return comdat;
}

bool
GlobalVariable::is_artificial() const {
  return get_llvm().hasGlobalUnnamedAddr();
}

bool
GlobalVariable::is_mangled() const {
  if(has_source_name())
    return get_source_name().size() != get_llvm_name().size();
  return false;
}

GlobalVariable&
GlobalVariable::make(const llvm::GlobalVariable& llvm_g, Module& module) {
  auto* global = new GlobalVariable(llvm_g, module);
  module.m_globals.emplace_back(global);
  module.vmap[&llvm_g] = global;

  return *global;
}

} // namespace lb

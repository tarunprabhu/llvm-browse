#include "Comdat.h"

#include "Function.h"
#include "GlobalVariable.h"
#include "Module.h"
#include "Value.h"

using llvm::cast;
using llvm::dyn_cast;
using llvm::isa;

namespace lb {

Comdat::Comdat(const llvm::Comdat& llvm_c,
               const llvm::GlobalObject& target,
               Module& module) :
    INavigable(EntityKind::Comdat),
    IWrapper<llvm::Comdat>(llvm_c, module),
    target(target) {
  set_tag(llvm_c.getName(), "$");
}

void
Comdat::set_self_llvm_defn(const LLVMRange& defn) {
  this->defn = defn;
}

const LLVMRange&
Comdat::get_self_llvm_defn() const {
  return defn;
}

const Value&
Comdat::get_target() const {
  return get_module().get(target);
}

template<typename T>
const T&
Comdat::get_target_as() const {
  return llvm::cast<T>(get_module().get(target));
}

template const Function&
Comdat::get_target_as<Function>() const;
template const GlobalVariable&
Comdat::get_target_as<GlobalVariable>() const;

Comdat&
Comdat::make(const llvm::Comdat& llvm_c,
             const llvm::GlobalObject& target,
             Module& module) {
  auto* comdat = new Comdat(llvm_c, target, module);
  module.m_comdats.emplace_back(comdat);
  module.cmap[&llvm_c] = comdat;

  return *comdat;
}

} // namespace lb
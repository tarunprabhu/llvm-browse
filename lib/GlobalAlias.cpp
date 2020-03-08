#include "GlobalAlias.h"
#include "Module.h"

#include <llvm/Support/Casting.h>

using llvm::cast;
using llvm::dyn_cast;
using llvm::isa;

namespace lb {

GlobalAlias::GlobalAlias(llvm::GlobalAlias& llvm_a, Module& module) :
    Value(EntityKind::GlobalAlias),
    INavigable(EntityKind::GlobalAlias), IWrapper<llvm::GlobalAlias>(llvm_a,
                                                                     module) {
  set_tag(llvm_a.getName(), "@");
}

bool GlobalAlias::has_source_info() const {
	return false;
}

bool GlobalAlias::has_source_name() const {
	return false;
}

llvm::StringRef GlobalAlias::get_source_name() const {
	return llvm::StringRef();
}

llvm::StringRef GlobalAlias::get_llvm_name() const {
	return get_llvm().getName();
}

} // namespace lb

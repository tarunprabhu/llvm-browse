#include "Argument.h"
#include "Function.h"
#include "Module.h"

#include <llvm/Support/Casting.h>

using llvm::cast;
using llvm::dyn_cast;
using llvm::isa;

namespace lb {

Argument::Argument(llvm::Argument& arg, Function& f, Module& module) :
    Value(Value::Kind::Argument),
    INavigable(), IWrapper<llvm::Argument>(arg, module), di(nullptr) {
  ;
}

void Argument::set_debug_info_node(const llvm::DILocalVariable* di) {
	this->di = di;
}

bool Argument::has_source_info() const {
	return di;
}

bool Argument::has_source_name() const {
	return get_source_name().size();
}

llvm::StringRef Argument::get_source_name() const {
	return has_source_info() ? di->getName() : llvm::StringRef("");
}

llvm::StringRef Argument::get_llvm_name() const {
	return get_tag();
}

bool Argument::is_artificial() const {
	return has_source_info() ? di->isArtificial() : false;
}

const Function&
Argument::get_function() const {
  return get_module().get(*get_llvm().getParent());
}

} // namespace lb

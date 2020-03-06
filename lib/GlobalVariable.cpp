#include "GlobalVariable.h"
#include "DIUtils.h"
#include "Logging.h"
#include "Module.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>

using llvm::cast;
using llvm::dyn_cast;
using llvm::isa;

namespace lb {

GlobalVariable::GlobalVariable(llvm::GlobalVariable& llvm_g, Module& module) :
    Value(Value::Kind::GlobalVariable),
    INavigable(), IWrapper<llvm::GlobalVariable>(llvm_g, module), di(nullptr) {
  if(llvm_g.hasName())
    set_tag(llvm_g.getName(), "@");
  else
    critical() << "Cannot set tag for unnamed global" << llvm_g << "\n";

  llvm::SmallVector<llvm::DIGlobalVariableExpression*, 4> dis;
  llvm_g.getDebugInfo(dis);
  if(dis.size() == 1) {
    di = dis[0]->getVariable();
    set_source_defn(SourceRange(di->getFilename().data(), di->getLine(), 1));
    source_name = DebugInfo::get_name(di);
    full_name = DebugInfo::get_full_name(di);
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

llvm::StringRef
GlobalVariable::get_source_name() const {
	return llvm::StringRef(source_name);
}

llvm::StringRef
GlobalVariable::get_llvm_name() const {
  return get_llvm().getName();
}

llvm::StringRef
GlobalVariable::get_full_name() const {
  return llvm::StringRef(full_name);
}

bool 
GlobalVariable::is_mangled() const {
	if(has_source_name())
		return get_source_name().size() != get_llvm_name().size();
	return false;
}

} // namespace lb

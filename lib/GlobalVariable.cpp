#include "GlobalVariable.h"
#include "Module.h"

#include <glib.h>

#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/Casting.h>

using llvm::cast;
using llvm::dyn_cast;
using llvm::isa;

namespace lb {

GlobalVariable::GlobalVariable(const llvm::GlobalVariable& llvm_g,
                               Module& module) :
    Value(Value::Kind::GlobalVariable, llvm_g, module) {
  if(llvm_g.hasName()) {
    set_tag(llvm_g.getName(), "@");
  } else {
    std::string buf;
    llvm::raw_string_ostream ss(buf);
    ss << llvm_g;
    g_critical("Cannot set tag for unnamed global: %s", buf.c_str());
  }
}

const llvm::GlobalVariable&
GlobalVariable::get_llvm() const {
  return cast<llvm::GlobalVariable>(llvm);
}

} // namespace lb

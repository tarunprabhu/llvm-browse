#include "Value.h"
#include "Module.h"

namespace lb {

Value::Value(Value::Kind kind, const llvm::Value& llvm, Module& module) :
    Navigable(), module(module), llvm(llvm), kind(kind) {
  ;
}

Value::Kind
Value::get_kind() const {
  return kind;
}
  
const Module&
Value::get_module() const {
  return module;
}

} // namespace lb

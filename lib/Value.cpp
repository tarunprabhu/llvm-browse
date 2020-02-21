#include "Value.h"

namespace lb {

Value::Value(Value::Kind kind, const llvm::Value& llvm, Module& module) :
    module(module), llvm(llvm), kind(kind) {
  ;
}

void
Value::init() {
  ;
}

Value::Kind
Value::get_kind() const {
  return kind;
}

const SourceRange&
Value::get_llvm_range() const {
  return llvm_range;
}

const SourceRange&
Value::get_source_range() const {
  return source_range;
}

const Module&
Value::get_module() const {
  return module;
}

} // namespace lb

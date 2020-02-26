#ifndef LLVM_BROWSE_GLOBAL_ALIAS_H
#define LLVM_BROWSE_GLOBAL_ALIAS_H

#include <llvm/IR/GlobalAlias.h>

#include "Value.h"

namespace lb {

class GlobalAlias : public Value {
public:
  GlobalAlias(const llvm::GlobalAlias& llvm_a, Module& module);
  virtual ~GlobalAlias() = default;

  virtual const llvm::GlobalAlias& get_llvm() const override;

public:
  static bool classof(const Value* v) {
    return v->get_kind() == Value::Kind::GlobalAlias;
  }
};

} // namespace lb

#endif // LLVM_BROWSE_GLOBAL_ALIAS_H

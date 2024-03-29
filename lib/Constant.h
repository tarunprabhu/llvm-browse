#ifndef LLVM_BROWSE_CONSTANT_H
#define LLVM_BROWSE_CONSTANT_H

#include <llvm/IR/Constant.h>
#include <llvm/IR/ModuleSlotTracker.h>

#include "Value.h"

namespace lb {

class Constant : public Value {
public:
  Constant(const llvm::Constant& llvm, Module& module);
  virtual ~Constant() = default;

  virtual const llvm::Constant& get_llvm() const override;

public:
  static bool classof(const Value* v) {
    return v->get_kind() == Value::Kind::Constant;
  }
};

} // namespace lb

#endif // LLVM_BROWSE_CONSTANT_H

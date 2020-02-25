#ifndef LLVM_BROWSE_CONSTANT_H
#define LLVM_BROWSE_CONSTANT_H

#include <llvm/IR/Constant.h>
#include <llvm/IR/ModuleSlotTracker.h>

#include "Module.h"
#include "Value.h"

namespace lb {

class Constant : public Value {
protected:
  virtual void init(llvm::ModuleSlotTracker& slots) override;
  
public:
  Constant(const llvm::Constant& llvm, Module& module);
  virtual ~Constant() = default;

  virtual const llvm::Constant& get_llvm() const override;

public:
  static bool classof(const Value* v) {
    return v->get_kind() == Value::Kind::Constant;
  }

public:
  friend class Module;
};

} // namespace lb

#endif // LLVM_BROWSE_CONSTANT_H

#ifndef LLVM_BROWSE_GLOBAL_VARIABLE_H
#define LLVM_BROWSE_GLOBAL_VARIABLE_H

#include <llvm/IR/ModuleSlotTracker.h>
#include <llvm/IR/GlobalVariable.h>

#include "Module.h"
#include "Value.h"

namespace lb {

class GlobalVariable : public Value {
protected:
  virtual void init(llvm::ModuleSlotTracker& slots) override;
  
public:
  GlobalVariable(const llvm::GlobalVariable& llvm_g,
                 Module& module);
  virtual ~GlobalVariable() = default;

  virtual const llvm::GlobalVariable& get_llvm() const override;

public:
  static bool classof(const Value* v) {
    return v->get_kind() == Value::Kind::GlobalVariable;
  }

public:
  friend class Module;
};

} // namespace lb

#endif // LLVM_BROWSE_GLOBAL_VARIABLE_H

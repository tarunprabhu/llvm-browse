#ifndef LLVM_BROWSE_GLOBAL_VARIABLE_H
#define LLVM_BROWSE_GLOBAL_VARIABLE_H

#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/ModuleSlotTracker.h>

#include "INavigable.h"
#include "IWrapper.h"
#include "Value.h"

namespace lb {

class Module;

class GlobalVariable :
    public Value,
    public INavigable,
    IWrapper<llvm::GlobalVariable> {
public:
  GlobalVariable(llvm::GlobalVariable& llvm_g, Module& module);
  virtual ~GlobalVariable() = default;

public:
  static bool classof(const Value* v) {
    return v->get_kind() == Value::Kind::GlobalVariable;
  }
};

} // namespace lb

#endif // LLVM_BROWSE_GLOBAL_VARIABLE_H

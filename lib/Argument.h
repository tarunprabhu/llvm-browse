#ifndef LLVM_BROWSE_ARGUMENT_H
#define LLVM_BROWSE_ARGUMENT_H

#include <llvm/IR/Function.h>
#include <llvm/IR/ModuleSlotTracker.h>

#include "Function.h"
#include "INavigable.h"
#include "IWrapper.h"
#include "Value.h"

namespace lb {

class Module;

class Argument :
    public Value,
    public INavigable,
    public IWrapper<llvm::Argument> {
public:
  Argument(llvm::Argument& llvm_arg, Module& module);
  virtual ~Argument() = default;

  const Function& get_function() const;

public:
  static bool classof(const Value* v) {
    return v->get_kind() == Value::Kind::Argument;
  }
};

} // namespace lb

#endif // LLVM_BROWSE_ARGUMENT_H

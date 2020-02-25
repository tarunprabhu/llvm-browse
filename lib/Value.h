#ifndef LLVM_BROWSE_VALUE_H
#define LLVM_BROWSE_VALUE_H

#include <llvm/IR/ModuleSlotTracker.h>
#include <llvm/IR/Value.h>

#include "Navigable.h"

namespace lb {

class Module;

class Value : public Navigable {
public:
  enum class Kind {
    Argument = 1,
    BasicBlock,
    Constant,
    Function,
    GlobalVariable,
    Instruction,
  };

protected:
  Module& module;
  const llvm::Value& llvm;
  Kind kind;

protected:
  Value(Kind kind, const llvm::Value& llvm, Module& module);

public:
  virtual ~Value() = default;

  Kind get_kind() const;
  const Module& get_module() const;
  virtual const llvm::Value& get_llvm() const = 0;
};

} // namespace lb

#endif // LLVM_BROWSE_VALUE_H

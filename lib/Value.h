#ifndef LLVM_BROWSE_VALUE_H
#define LLVM_BROWSE_VALUE_H

#include <llvm/ADT/iterator_range.h>
#include <llvm/IR/ModuleSlotTracker.h>
#include <llvm/IR/Value.h>

#include <vector>

#include "IWrapper.h"

namespace lb {

class Module;

class Value {
public:
  enum class Kind {
    Argument = 1,
    BasicBlock,
    Constant,
    Function,
    GlobalAlias,
    GlobalVariable,
    Instruction,
  };

protected:
  Kind kind;

protected:
  Value(Kind kind);

public:
  virtual ~Value() = default;

  Kind get_kind() const;
};

} // namespace lb

#endif // LLVM_BROWSE_VALUE_H

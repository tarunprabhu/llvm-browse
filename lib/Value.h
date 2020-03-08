#ifndef LLVM_BROWSE_VALUE_H
#define LLVM_BROWSE_VALUE_H

#include <llvm/ADT/iterator_range.h>
#include <llvm/IR/ModuleSlotTracker.h>
#include <llvm/IR/Value.h>

#include <vector>

#include "Entities.h"
#include "IWrapper.h"

namespace lb {

class Module;

class Value {
protected:
  EntityKind kind;

protected:
  Value(EntityKind kind);

public:
  virtual ~Value() = default;

  EntityKind get_kind() const;
};

} // namespace lb

#endif // LLVM_BROWSE_VALUE_H

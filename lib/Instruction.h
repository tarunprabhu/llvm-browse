#ifndef LLVM_BROWSE_INSTRUCTION_H
#define LLVM_BROWSE_INSTRUCTION_H

#include <llvm/ADT/iterator_range.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/ModuleSlotTracker.h>

#include <vector>

#include "BasicBlock.h"
#include "Value.h"

namespace lb {

class Instruction : public Value {
protected:
  std::vector<const Value*> ops;

public:
  using Iterator = decltype(ops)::const_iterator;

public:
  Instruction(const llvm::Instruction& llvm_i, Module& module);
  virtual ~Instruction() = default;

  Iterator begin() const;
  Iterator end() const;
  llvm::iterator_range<Iterator> operands() const;
  const BasicBlock& get_block() const;
  const Function& get_function() const;
  virtual const llvm::Instruction& get_llvm() const override;

public:
  static bool classof(const Value* v) {
    return v->get_kind() == Value::Kind::Instruction;
  }

public:
  friend class BasicBlock;
};

} // namespace lb

#endif // LLVM_BROWSE_INSTRUCTION_H

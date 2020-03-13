#ifndef LLVM_BROWSE_INSTRUCTION_H
#define LLVM_BROWSE_INSTRUCTION_H

#include <llvm/ADT/iterator_range.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/ModuleSlotTracker.h>

#include <vector>

#include "INavigable.h"
#include "IWrapper.h"
#include "SourceRange.h"
#include "Value.h"

namespace lb {

class BasicBlock;
class Function;

class Instruction :
    public Value,
    public INavigable,
    public IWrapper<llvm::Instruction> {
protected:
  std::vector<SourceRange> ops;
  BasicBlock& parent;
  llvm::DebugLoc di;

public:
  using Iterator = decltype(ops)::const_iterator;

protected:
  Instruction(const llvm::Instruction& llvm_i,
              BasicBlock& bb,
              Function& f,
              Module& module);
  BasicBlock& get_block();

public:
  Instruction()              = delete;
  Instruction(Instruction&)  = delete;
  Instruction(Instruction&&) = delete;
  virtual ~Instruction()     = default;

  void add_operand(const SourceRange& = SourceRange());

  bool has_source_info() const;
  llvm::StringRef get_llvm_name() const;
  SourceRange get_operand(unsigned i) const;
  Iterator begin() const;
  Iterator end() const;
  llvm::iterator_range<Iterator> operands() const;
  const BasicBlock& get_block() const;
  const Function& get_function() const;
  bool is_llvm_lifetime_inst() const;
  bool is_llvm_debug_inst() const;

public:
  static bool classof(const Value* v) {
    return v->get_kind() == EntityKind::Instruction;
  }

  static bool classof(const INavigable* v) {
    return v->get_kind() == EntityKind::Instruction;
  }

  static Instruction& make(const llvm::Instruction& llvm_i,
                           BasicBlock& bb,
                           Function& f,
                           Module& module);
};

} // namespace lb

#endif // LLVM_BROWSE_INSTRUCTION_H

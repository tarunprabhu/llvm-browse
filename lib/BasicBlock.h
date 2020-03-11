#ifndef LLVM_BROWSE_BASIC_BLOCK_H
#define LLVM_BROWSE_BASIC_BLOCK_H

#include <llvm/ADT/iterator_range.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/ModuleSlotTracker.h>

#include <memory>
#include <vector>

#include "INavigable.h"
#include "IWrapper.h"
#include "Instruction.h"
#include "Iterator.h"
#include "Value.h"

namespace lb {

class Function;
class Module;

class BasicBlock :
    public Value,
    public INavigable,
    public IWrapper<llvm::BasicBlock> {
protected:
  std::vector<std::unique_ptr<Instruction>> m_insts;
  Function& parent;

public:
  using InstIterator = DerefIterator<decltype(m_insts)::const_iterator>;

protected:
  BasicBlock(const llvm::BasicBlock& llvm_bb, Function& f, Module& module);
  Function& get_function();

public:
  BasicBlock()             = delete;
  BasicBlock(BasicBlock&)  = delete;
  BasicBlock(BasicBlock&&) = delete;
  virtual ~BasicBlock()    = default;

  InstIterator begin() const;
  InstIterator end() const;
  llvm::iterator_range<InstIterator> instructions() const;
  const Function& get_function() const;
  bool has_source_info() const;

public:
  static bool classof(const Value* v) {
    return v->get_kind() == EntityKind::BasicBlock;
  }

  static bool classof(const INavigable* v) {
    return v->get_kind() == EntityKind::BasicBlock;
  }

  static BasicBlock&
  make(const llvm::BasicBlock& llvm_bb, Function& f, Module& module);

public:
  friend Instruction& Instruction::make(const llvm::Instruction& llvm_i,
                                        BasicBlock& bb,
                                        Function& f,
                                        Module& module);
};

} // namespace lb

#endif // LLVM_BROWSE_BASIC_BLOCK_H

#ifndef LLVM_BROWSE_VALUE_H
#define LLVM_BROWSE_VALUE_H

#include <llvm/ADT/iterator_range.h>
#include <llvm/IR/ModuleSlotTracker.h>
#include <llvm/IR/Value.h>

#include <vector>

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
    GlobalAlias,
    GlobalVariable,
    Instruction,
  };

protected:
  Module& module;
  const llvm::Value& llvm;
  Kind kind;
  std::vector<SourceRange> m_uses;

public:
  using Iterator = decltype(m_uses)::const_iterator;
  
protected:
  Value(Kind kind, const llvm::Value& llvm, Module& module);

public:
  virtual ~Value() = default;

  void sort_uses();
  void add_use(const SourceRange& range);

  Iterator begin() const;
  Iterator end() const;
  llvm::iterator_range<Iterator> uses() const;
  unsigned get_num_uses() const;
  Kind get_kind() const;
  const Module& get_module() const;
  virtual const llvm::Value& get_llvm() const = 0;
};

} // namespace lb

#endif // LLVM_BROWSE_VALUE_H

#ifndef LLVM_BROWSE_VALUE_H
#define LLVM_BROWSE_VALUE_H

#include <llvm/IR/Value.h>

#include "SourceRange.h"

namespace lb {

class Module;

class Value {
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
  SourceRange llvm_range;
  SourceRange source_range;

protected:
  virtual void init();
  Value(Kind, const llvm::Value&, Module&);

public:
  virtual ~Value() = default;

  Kind get_kind() const;
  const SourceRange& get_llvm_range() const;
  const SourceRange& get_source_range() const;
  const Module& get_module() const;
  virtual const llvm::Value& get_llvm() const = 0;
};

} // namespace lb

#endif // LLVM_BROWSE_VALUE_H

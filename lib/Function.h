#ifndef LLVM_BROWSE_FUNCTION_H
#define LLVM_BROWSE_FUNCTION_H

#include <llvm/ADT/iterator_range.h>
#include <llvm/IR/Function.h>

#include <vector>

#include "Module.h"
#include "Value.h"

namespace lb {

class Argument;
class BasicBlock;

class Function : public Value {
protected:
  std::vector<const Argument*> args;
  std::vector<const BasicBlock*> bbs;

public:
  using Iterator = decltype(bbs)::const_iterator;

protected:
  virtual void init() override;

public:
  Function(const llvm::Function&, Module&);
  virtual ~Function() = default;

  Iterator begin() const;
  Iterator end() const;
  llvm::iterator_range<Iterator> blocks() const;

  virtual const llvm::Function& get_llvm() const override;

public:
  static bool classof(const Value* v) {
    return v->get_kind() == Value::Kind::Function;
  }

public:
  friend class Module;
};

} // namespace lb

#endif // LLVM_BROWSE_FUNCTION_H

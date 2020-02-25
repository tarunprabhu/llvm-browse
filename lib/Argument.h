#ifndef LLVM_BROWSE_ARGUMENT_H
#define LLVM_BROWSE_ARGUMENT_H

#include <llvm/IR/Function.h>
#include <llvm/IR/ModuleSlotTracker.h>

#include "Function.h"
#include "Value.h"

namespace lb {

class Argument : public Value {
protected:
  virtual void init(llvm::ModuleSlotTracker& slots) override;
  
public:
  Argument(const llvm::Argument& llvm_arg,
           Module& module);
  virtual ~Argument() = default;

  const Function& get_function() const;
  virtual const llvm::Argument& get_llvm() const override;

public:
  static bool classof(const Value* v) {
    return v->get_kind() == Value::Kind::Argument;
  }

public:
  friend class Function;
};

} // namespace lb

#endif // LLVM_BROWSE_ARGUMENT_H

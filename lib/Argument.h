#ifndef LLVM_BROWSE_ARGUMENT_H
#define LLVM_BROWSE_ARGUMENT_H

#include <llvm/IR/Function.h>

#include "Value.h"

namespace lb {

class Function;

class Argument : public Value {
public:
  Argument(const llvm::Argument&, Module&);
  virtual ~Argument() = default;

  const Function& get_function() const;
  virtual const llvm::Argument& get_llvm() const override;

public:
  static bool classof(const Value* v) {
    return v->get_kind() == Value::Kind::Argument;
  }
};

} // namespace lb

#endif // LLVM_BROWSE_ARGUMENT_H

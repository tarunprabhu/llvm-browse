#ifndef LLVM_BROWSE_GLOBAL_VARIABLE_H
#define LLVM_BROWSE_GLOBAL_VARIABLE_H

#include <llvm/IR/GlobalVariable.h>

#include "Module.h"
#include "Value.h"

namespace lb {

class GlobalVariable : public Value {
public:
  GlobalVariable(const llvm::GlobalVariable&, Module&);
  virtual ~GlobalVariable() = default;

  virtual const llvm::GlobalVariable& get_llvm() const override;

public:
  static bool classof(const Value* v) {
    return v->get_kind() == Value::Kind::GlobalVariable;
  }

public:
  friend class Module;
};

} // namespace lb

#endif // LLVM_BROWSE_GLOBAL_VARIABLE_H

#ifndef LLVM_BROWSE_GLOBAL_ALIAS_H
#define LLVM_BROWSE_GLOBAL_ALIAS_H

#include <llvm/IR/GlobalAlias.h>

#include "INavigable.h"
#include "IWrapper.h"
#include "Typedefs.h"
#include "Value.h"

namespace lb {

class alignas(ALIGN_OBJ) GlobalAlias :
    public Value,
    public INavigable,
    public IWrapper<llvm::GlobalAlias> {
public:
  GlobalAlias(llvm::GlobalAlias& llvm_a, Module& module);
  virtual ~GlobalAlias() = default;

  // This will not have any source information
  bool has_source_info() const;
  bool has_source_name() const;
  llvm::StringRef get_source_name() const;
  llvm::StringRef get_llvm_name() const;

public:
  static bool classof(const Value* v) {
    return v->get_kind() == EntityKind::GlobalAlias;
  }

  static bool classof(const INavigable* v) {
  	return v->get_kind() == EntityKind::GlobalAlias;
  }
};

} // namespace lb

#endif // LLVM_BROWSE_GLOBAL_ALIAS_H

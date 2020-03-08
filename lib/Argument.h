#ifndef LLVM_BROWSE_ARGUMENT_H
#define LLVM_BROWSE_ARGUMENT_H

#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/ModuleSlotTracker.h>

#include "Function.h"
#include "INavigable.h"
#include "IWrapper.h"
#include "Value.h"

namespace lb {

class Module;

class Argument :
    public Value,
    public INavigable,
    public IWrapper<llvm::Argument> {
protected:
	const llvm::DILocalVariable* di;

public:
  Argument(llvm::Argument& llvm_arg, Function& f, Module& module);
  virtual ~Argument() = default;

	void set_debug_info_node(const llvm::DILocalVariable* di);

  const Function& get_function() const;
  bool has_source_info() const;
  bool has_source_name() const;
  llvm::StringRef get_source_name() const;
  llvm::StringRef get_llvm_name() const;
  bool is_artificial() const;

public:
  static bool classof(const Value* v) {
    return v->get_kind() == EntityKind::Argument;
  }

  static bool classof(const INavigable* v) {
  	return v->get_kind() == EntityKind::Argument;
  }
};

} // namespace lb

#endif // LLVM_BROWSE_ARGUMENT_H

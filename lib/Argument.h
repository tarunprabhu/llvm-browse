#ifndef LLVM_BROWSE_ARGUMENT_H
#define LLVM_BROWSE_ARGUMENT_H

#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/ModuleSlotTracker.h>

#include "INavigable.h"
#include "IWrapper.h"
#include "Value.h"

namespace lb {

class Function;
class Module;

class Argument :
    public Value,
    public INavigable,
    public IWrapper<llvm::Argument> {
protected:
  Function& parent;
  const llvm::DILocalVariable* di;
  std::string source_name;

protected:
  Argument(const llvm::Argument& llvm_arg, Function& f, Module& module);
  Function& get_function();

public:
  Argument()           = delete;
  Argument(Argument&)  = delete;
  Argument(Argument&&) = delete;
  virtual ~Argument()  = default;

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

  static Argument&
  make(const llvm::Argument& llvm_arg, Function& f, Module& module);
};

} // namespace lb

#endif // LLVM_BROWSE_ARGUMENT_H

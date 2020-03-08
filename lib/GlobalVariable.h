#ifndef LLVM_BROWSE_GLOBAL_VARIABLE_H
#define LLVM_BROWSE_GLOBAL_VARIABLE_H

#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/ModuleSlotTracker.h>

#include "INavigable.h"
#include "IWrapper.h"
#include "Typedefs.h"
#include "Value.h"

namespace lb {

class Comdat;
class Module;

class alignas(ALIGN_OBJ) GlobalVariable :
    public Value,
    public INavigable,
    IWrapper<llvm::GlobalVariable> {
protected:
	Comdat* comdat;
  const llvm::DIGlobalVariable* di;
  std::string source_name;
  std::string full_name;

public:
  GlobalVariable(llvm::GlobalVariable& llvm_g, Module& module);
  virtual ~GlobalVariable() = default;

  bool has_source_info() const;
  bool has_source_name() const;
  bool has_full_name() const;
  llvm::StringRef get_source_name() const;
  llvm::StringRef get_llvm_name() const;
  llvm::StringRef get_full_name() const;
  const Comdat* get_comdat() const;
  bool is_artificial() const;
  bool is_mangled() const;

public:
  static bool classof(const Value* v) {
    return v->get_kind() == EntityKind::GlobalVariable;
  }
 	
  static bool classof(const INavigable* v) {
  	return v->get_kind() == EntityKind::GlobalVariable;
  }
};

} // namespace lb

#endif // LLVM_BROWSE_GLOBAL_VARIABLE_H

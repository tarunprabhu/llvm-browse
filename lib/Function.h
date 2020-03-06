#ifndef LLVM_BROWSE_FUNCTION_H
#define LLVM_BROWSE_FUNCTION_H

#include <llvm/ADT/iterator_range.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/ModuleSlotTracker.h>

#include <vector>

#include "INavigable.h"
#include "IWrapper.h"
#include "Typedefs.h"
#include "Value.h"

namespace lb {

class Argument;
class BasicBlock;
class Module;

class alignas(ALIGN_OBJ) Function :
    public Value,
    public INavigable,
    public IWrapper<llvm::Function> {
protected:
  std::vector<Argument*> args;
  std::vector<BasicBlock*> bbs;
  const llvm::DISubprogram* di;
  std::string source_name;
  std::string full_name;

public:
  using ArgIterator = decltype(args)::const_iterator;
  using Iterator    = decltype(bbs)::const_iterator;

public:
  Function(llvm::Function& llvm_f, Module& module);
  virtual ~Function() = default;

  bool has_source_info() const;
  bool has_source_name() const;
  bool has_full_name() const;
  llvm::StringRef get_source_name() const;
  llvm::StringRef get_llvm_name() const;
  llvm::StringRef get_full_name() const;
  bool is_mangled() const;
  bool is_artificial() const;
  bool is_defined() const;

  // C++-specific flags
  bool is_method() const;
  bool is_virtual() const;
  bool is_pure_virtual() const;
  bool is_public() const;
  bool is_private() const;
  bool is_protected() const;

  Argument& get_arg(unsigned i);
  const Argument& get_arg(unsigned i) const;

  Iterator begin() const;
  Iterator end() const;
  llvm::iterator_range<Iterator> blocks() const;
  ArgIterator arg_begin() const;
  ArgIterator arg_end() const;
  llvm::iterator_range<ArgIterator> arguments() const;

public:
  static bool classof(const Value* v) {
    return v->get_kind() == Value::Kind::Function;
  }
};

} // namespace lb

#endif // LLVM_BROWSE_FUNCTION_H

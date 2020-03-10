#ifndef LLVM_BROWSE_FUNCTION_H
#define LLVM_BROWSE_FUNCTION_H

#include <llvm/ADT/iterator_range.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/ModuleSlotTracker.h>

#include <memory>
#include <vector>

#include "Argument.h"
#include "BasicBlock.h"
#include "INavigable.h"
#include "IWrapper.h"
#include "Iterator.h"
#include "Typedefs.h"
#include "Value.h"

namespace lb {

class Comdat;
class Module;

class alignas(ALIGN_OBJ) Function :
    public Value,
    public INavigable,
    public IWrapper<llvm::Function> {
protected:
  std::vector<std::unique_ptr<Argument>> m_args;
  std::vector<std::unique_ptr<BasicBlock>> m_blocks;
  const Comdat* comdat;
  const llvm::DISubprogram* di;
  std::string source_name;
  std::string full_name;

public:
  using ArgIterator   = DerefIterator<decltype(m_args)::const_iterator>;
  using BlockIterator = DerefIterator<decltype(m_blocks)::const_iterator>;

protected:
  Function(const llvm::Function& llvm_f, Module& module);

public:
  Function()           = delete;
  Function(Function&)  = delete;
  Function(Function&&) = delete;
  virtual ~Function()  = default;

  bool has_source_info() const;
  bool has_source_name() const;
  bool has_full_name() const;
  llvm::StringRef get_source_name() const;
  llvm::StringRef get_llvm_name() const;
  llvm::StringRef get_full_name() const;
  const Comdat* get_comdat() const;
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

  BlockIterator begin() const;
  BlockIterator end() const;
  llvm::iterator_range<BlockIterator> blocks() const;
  ArgIterator arg_begin() const;
  ArgIterator arg_end() const;
  llvm::iterator_range<ArgIterator> arguments() const;

public:
  static bool classof(const Value* v) {
    return v->get_kind() == EntityKind::Function;
  }

  static bool classof(const INavigable* v) {
    return v->get_kind() == EntityKind::Function;
  }

  static Function& make(const llvm::Function& llvm_f, Module& module);

public:
  friend Argument&
  Argument::make(const llvm::Argument& llvm_a, Function& f, Module& module);
  friend BasicBlock&
  BasicBlock::make(const llvm::BasicBlock&, Function& f, Module& module);
};

} // namespace lb

#endif // LLVM_BROWSE_FUNCTION_H

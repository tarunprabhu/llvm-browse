#ifndef LLVM_BROWSE_MODULE_H
#define LLVM_BROWSE_MODULE_H

#include <llvm/ADT/iterator_range.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/ModuleSlotTracker.h>
#include <llvm/Support/MemoryBuffer.h>

#include <map>
#include <vector>

#include "Comdat.h"
#include "Errors.h"
#include "LLVMRange.h"
#include "MDNode.h"
#include "Parser.h"
#include "StructType.h"
#include "Typedefs.h"
#include "Value.h"

namespace lb {

class Argument;
class BasicBlock;
class Instruction;
class Function;
class GlobalAlias;
class GlobalVariable;
class INavigable;
class Value;

// Wrapper class around an LLVM module. The wrappers around the LLVM classes
// contains "LLVM source code" information, mainly just line and column
// numbers. Keeping it in here instead of separately so that if we need
// additional flags for the entities, they can be kept in one place that
// makes sense
//
class alignas(ALIGN_OBJ) Module {
protected:
  // Managed memory for objects that will always live for the duration of
  // the object but will never be touched directly
  std::unique_ptr<llvm::LLVMContext> context;
  std::unique_ptr<llvm::Module> llvm;
  std::vector<std::unique_ptr<Comdat>> comdat_ptrs;
  std::vector<std::unique_ptr<Value>> value_ptrs;
  std::vector<std::unique_ptr<MDNode>> mdnode_ptrs;
  std::vector<std::unique_ptr<StructType>> struct_ptrs;
  std::unique_ptr<llvm::MemoryBuffer> buffer;

  // When looking up anything, these will be actually returned
  std::vector<const Comdat*> m_comdats;
  std::vector<const Function*> m_functions;
  std::vector<const GlobalVariable*> m_globals;
  std::vector<const GlobalAlias*> m_aliases;
  std::vector<const MDNode*> m_metadata;
  std::vector<const StructType*> m_structs;

  std::map<const llvm::Comdat*, Comdat*> cmap;
  std::map<const llvm::MDNode*, MDNode*> mmap;
  std::map<llvm::StructType*, StructType*> tmap;
  std::map<const llvm::Value*, Value*> vmap;

public:
  using AliasIterator    = decltype(m_aliases)::const_iterator;
  using ComdatIterator   = decltype(m_comdats)::const_iterator;
  using FunctionIterator = decltype(m_functions)::const_iterator;
  using GlobalIterator   = decltype(m_globals)::const_iterator;
  using MetadataIterator = decltype(m_metadata)::const_iterator;
  using StructIterator   = decltype(m_structs)::const_iterator;

protected:
  Module(std::unique_ptr<llvm::Module> module,
         std::unique_ptr<llvm::LLVMContext> context,
         std::unique_ptr<llvm::MemoryBuffer> mbuf);

  template<typename T = Value>
  T& get(const llvm::Value& llvm) {
    return *llvm::cast<T>(vmap.at(&llvm));
  }

  template<typename T = Value>
  T& get(const llvm::Value* llvm) {
    return llvm::cast<T>(vmap.at(llvm));
  }

  template<typename T = Value>
  const T& get(const llvm::Value& llvm) const {
    return *llvm::cast<T>(vmap.at(&llvm));
  }

  template<typename T = Value>
  const T* get(const llvm::Value* llvm) const {
    return llvm::cast<T>(vmap.at(llvm));
  }

  template<typename T,
           typename LLVM,
           typename... ArgsT,
           std::enable_if_t<!std::is_pointer<LLVM>::value, int> = 0>
  T& add(LLVM& llvm, ArgsT&&... args) {
    T* ptr = new T(llvm, args..., *this);
    value_ptrs.emplace_back(ptr);
    vmap[&llvm] = ptr;
    return *ptr;
  }

  bool check_range(const LLVMRange& range, llvm::StringRef tag) const;
  bool check_uses(const INavigable& navigable) const;
  bool check_navigable(const INavigable& navigable) const;

public:
  Module(const Module&)  = delete;
  Module(const Module&&) = delete;
  virtual ~Module()      = default;

  bool contains(const llvm::Value& llvm) const;
  bool contains(const llvm::MDNode& llvm) const;

  llvm::MemoryBufferRef get_code_buffer() const;
  llvm::StringRef get_code() const;

  // These should be refactored at some point so they are not public
  Argument& add(llvm::Argument& llvm, Function& f);
  BasicBlock& add(llvm::BasicBlock& llvm, Function& f);
  Comdat& add(llvm::Comdat&, llvm::GlobalObject& g);
  Function& add(llvm::Function& llvm);
  GlobalAlias& add(llvm::GlobalAlias& llvm);
  GlobalVariable& add(llvm::GlobalVariable& llvm);
  Instruction& add(llvm::Instruction& llvm, Function& f);
  MDNode& add(llvm::MDNode& llvm, unsigned slot);
  StructType& add(llvm::StructType* llvm);

  Argument& get(const llvm::Argument& llvm);
  BasicBlock& get(const llvm::BasicBlock& llvm);
  Comdat& get(const llvm::Comdat& llvm);
  Instruction& get(const llvm::Instruction& llvm);
  Function& get(const llvm::Function& llvm);
  GlobalAlias& get(const llvm::GlobalAlias& llvm);
  GlobalVariable& get(const llvm::GlobalVariable& llvm);
  MDNode& get(const llvm::MDNode& llvm);
  StructType& get(llvm::StructType* llvm);
  Value& get(const llvm::Value& llvm);

  const StructType& get(llvm::StructType* llvm) const;
  const Argument& get(const llvm::Argument& llvm) const;
  const BasicBlock& get(const llvm::BasicBlock& llvm) const;
  const Comdat& get(const llvm::Comdat& llvm) const;
  const Instruction& get(const llvm::Instruction& llvm) const;
  const Function& get(const llvm::Function& llvm) const;
  const GlobalAlias& get(const llvm::GlobalAlias& llvm) const;
  const GlobalVariable& get(const llvm::GlobalVariable& llvm) const;
  const MDNode& get(const llvm::MDNode& llvm) const;
  const Value& get(const llvm::Value& llvm) const;

  llvm::iterator_range<AliasIterator> aliases() const;
  llvm::iterator_range<ComdatIterator> comdats() const;
  llvm::iterator_range<FunctionIterator> functions() const;
  llvm::iterator_range<GlobalIterator> globals() const;
  llvm::iterator_range<MetadataIterator> metadata() const;
  llvm::iterator_range<StructIterator> structs() const;

  unsigned get_num_aliases() const;
  unsigned get_num_comdats() const;
  unsigned get_num_functions() const;
  unsigned get_num_globals() const;
  unsigned get_num_metadata() const;
  unsigned get_num_structs() const;

  llvm::Module& get_llvm();
  const llvm::Module& get_llvm() const;

  bool check_top_level() const;
  bool check_all(bool metadata) const;

  explicit operator bool() const {
    return llvm.get();
  }

public:
  static std::unique_ptr<Module> create(const std::string& file);
};

} // namespace lb

#endif // LLVM_BROWSE_MODULE_H

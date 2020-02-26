#ifndef LLVM_BROWSE_MODULE_H
#define LLVM_BROWSE_MODULE_H

#include <llvm/ADT/iterator_range.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/ModuleSlotTracker.h>

#include <map>
#include <vector>

#include "Errors.h"
#include "SourceFile.h"
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
class Navigable;
class Value;

// Wrapper class around an LLVM module. The wrappers around the LLVM classes
// contains "LLVM source code" information, mainly just line and column
// numbers. Keeping it in here instead of separately so that if we need
// additional flags for the entities, they can be kept in one place that
// makes sense
//
class Module {
protected:
  llvm::LLVMContext& context;
  std::unique_ptr<llvm::Module> llvm;
  std::vector<std::unique_ptr<Value>> values;
  std::vector<std::unique_ptr<StructType>> struct_types;
  std::map<BufferId, std::unique_ptr<llvm::MemoryBuffer>> buffers;
  std::vector<const Function*> m_functions;
  std::vector<const GlobalVariable*> m_globals;
  std::vector<const GlobalAlias*> m_aliases;
  std::vector<const StructType*> m_structs;
  std::map<const llvm::Value*, Value*> vmap;
  std::map<llvm::StructType*, StructType*> tmap;
  bool valid;

public:
  using AliasIterator      = decltype(m_aliases)::const_iterator;
  using FunctionIterator   = decltype(m_functions)::const_iterator;
  using GlobalIterator     = decltype(m_globals)::const_iterator;
  using StructTypeIterator = decltype(m_structs)::const_iterator;

protected:
  Module(std::unique_ptr<llvm::Module> module, llvm::LLVMContext& context);
  BufferId get_next_available_id();

  template<typename T = Value>
  T& get(const llvm::Value& llvm) {
    return *llvm::cast<T>(vmap.at(&llvm));
  }

  template<typename T = Value>
  const T& get(const llvm::Value& llvm) const {
    return *llvm::cast<T>(vmap.at(&llvm));
  }

  bool check_range(const SourceRange& range, llvm::StringRef tag) const;
  bool check_value(const Value& value) const;
  bool check_navigable(const Navigable& navigable) const;

public:
  Module(const Module&)  = delete;
  Module(const Module&&) = delete;
  virtual ~Module()      = default;

  bool contains_main() const;
  bool contains(const llvm::Value& llvm) const;

  BufferId add_main(const std::string& file);
  BufferId add_main(std::unique_ptr<llvm::MemoryBuffer> buffer);
  BufferId add_file(const std::string& file);

  StructType& add(llvm::StructType* llvm);
  Function& add(const llvm::Function& llvm);
  GlobalAlias& add(const llvm::GlobalAlias& llvm);
  GlobalVariable& add(const llvm::GlobalVariable& llvm);

  template<typename T,
           typename LLVM,
           std::enable_if_t<!std::is_pointer<LLVM>::value, int> = 0>
  T& add(const LLVM& llvm) {
    T* ptr = new T(llvm, *this);
    values.emplace_back(ptr);
    vmap[&llvm] = ptr;
    return *ptr;
  }

  StructType& get(llvm::StructType* llvm);
  Argument& get(const llvm::Argument& llvm);
  BasicBlock& get(const llvm::BasicBlock& llvm);
  Instruction& get(const llvm::Instruction& llvm);
  Function& get(const llvm::Function& llvm);
  GlobalAlias& get(const llvm::GlobalAlias& llvm);
  GlobalVariable& get(const llvm::GlobalVariable& llvm);
  Value& get(const llvm::Value& llvm);
  const StructType& get(llvm::StructType* llvm) const;
  const Argument& get(const llvm::Argument& llvm) const;
  const BasicBlock& get(const llvm::BasicBlock& llvm) const;
  const Instruction& get(const llvm::Instruction& llvm) const;
  const Function& get(const llvm::Function& llvm) const;
  const GlobalAlias& get(const llvm::GlobalAlias& llvm) const;
  const GlobalVariable& get(const llvm::GlobalVariable& llvm) const;
  const Value& get(const llvm::Value& llvm) const;

  template<typename T, typename LLVM>
  const T& get_or_insert(const LLVM& llvm) {
    if(contains(llvm))
      return get<T>(llvm);
    return add<T>(llvm);
  }

  llvm::MemoryBufferRef get_buffer(BufferId id = get_main_id()) const;
  llvm::StringRef get_contents(BufferId id = get_main_id()) const;

  llvm::iterator_range<AliasIterator> aliases() const;
  llvm::iterator_range<FunctionIterator> functions() const;
  llvm::iterator_range<GlobalIterator> globals() const;
  llvm::iterator_range<StructTypeIterator> structs() const;

  llvm::Module& get_llvm();
  const llvm::Module& get_llvm() const;

  bool check_top_level() const;
  bool check_all(bool metadata) const;

  explicit operator bool() const {
    return valid;
  }

public:
  static std::unique_ptr<Module> create(const std::string& file,
                                        llvm::LLVMContext& context);

  static constexpr BufferId get_invalid_id() {
    return -1;
  }

  static constexpr BufferId get_main_id() {
    return 0;
  }
};

} // namespace lb

#endif // LLVM_BROWSE_MODULE_H

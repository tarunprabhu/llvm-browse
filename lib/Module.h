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
#include "Value.h"

namespace lb {

class Function;
class GlobalVariable;
class Value;

using BufferId = uint64_t;

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
  std::vector<const Function*> fptrs;
  std::vector<const GlobalVariable*> gptrs;
  std::vector<const StructType*> sptrs;
  std::map<const llvm::Value*, Value*> vmap;
  std::map<llvm::StructType*, StructType*> tmap;
  bool valid;
  
public:
  using FunctionIterator   = decltype(fptrs)::const_iterator;
  using GlobalIterator     = decltype(gptrs)::const_iterator;
  using StructTypeIterator = decltype(sptrs)::const_iterator;

protected:
  static constexpr BufferId get_invalid_id() {
    return -1;
  }
  
  static constexpr BufferId get_main_id() {
    return 0;
  }

protected:
  Module(std::unique_ptr<llvm::Module> module, llvm::LLVMContext& context);
  BufferId get_next_available_id();  

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
  
  template<typename T = Value>
  T& get(const llvm::Value& llvm) {
    return *llvm::cast<T>(vmap.at(&llvm));
  }

  template<typename T = Value>
  const T& get(const llvm::Value& llvm) const {
    return *llvm::cast<T>(vmap.at(&llvm));
  }

  const StructType& get(llvm::StructType* llvm) const;

  template<typename T, typename LLVM>
  const T& get_or_insert(const LLVM& llvm) {
    if(contains(llvm))
      return get<T>(llvm);
    return add<T>(llvm);
  }

  llvm::MemoryBufferRef get_buffer(BufferId id = get_main_id()) const;
  llvm::StringRef get_contents(BufferId id = get_main_id()) const;

  llvm::iterator_range<FunctionIterator> functions() const;
  llvm::iterator_range<GlobalIterator> globals() const;
  llvm::iterator_range<StructTypeIterator> structs() const;

  llvm::Module& get_llvm();
  const llvm::Module& get_llvm() const;

  explicit operator bool() const {
    return valid;
  }

public:
  static std::unique_ptr<Module> create(const std::string& file,
                                        llvm::LLVMContext& context);
};

} // namespace lb

#endif // LLVM_BROWSE_MODULE_H

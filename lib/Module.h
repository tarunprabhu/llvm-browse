#ifndef LLVM_BROWSE_MODULE_H
#define LLVM_BROWSE_MODULE_H

#include <llvm/ADT/iterator_range.h>
#include <llvm/IR/Module.h>

#include <map>
#include <vector>

#include "Errors.h"
#include "SourceFile.h"
#include "Value.h"

namespace lb {

class Function;
class GlobalVariable;
class Value;

// Wrapper class around an LLVM module. The wrappers around the LLVM classes
// contains "LLVM source code" information, mainly just line and column
// numbers. Keeping it in here instead of separately so that if we need
// additional flags for the entities, they can be kept in one place that
// makes sense
//
class Module {
protected:
  llvm::LLVMContext context;
  std::unique_ptr<llvm::Module> llvm;
  std::vector<const Function*> funcs;
  std::vector<const GlobalVariable*> globs;
  std::map<const llvm::Value*, Value*> vmap;
  std::vector<std::unique_ptr<Value>> values;
  // This may not actually be a source file because we might support loading
  // LLVM bitcode files at some point in which case, this will be the human
  // readable representation of it that is generated on the fly
  SourceFile src;
  bool valid;

public:
  using FunctionIterator = decltype(funcs)::const_iterator;
  using GlobalIterator   = decltype(globs)::const_iterator;

public:
  Module(const std::string&);
  Module(const Module&)  = delete;
  Module(const Module&&) = delete;
  virtual ~Module()      = default;

  ErrorCode parse(const std::string&);

  bool contains(const llvm::Value& llvm) const {
    return vmap.find(&llvm) != vmap.end();
  }

  template<typename T, typename LLVM>
  const T& add(const LLVM& llvm) {
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

  template<typename T, typename LLVM>
  const T& get_or_insert(const LLVM& llvm) {
    if(contains(llvm))
      return get<T>(llvm);
    return add<T>(llvm);
  }

  llvm::iterator_range<FunctionIterator> functions() const;
  llvm::iterator_range<GlobalIterator> globals() const;
  explicit operator bool() const {
    return valid;
  }
};

} // namespace lb

#endif // LLVM_BROWSE_MODULE_H

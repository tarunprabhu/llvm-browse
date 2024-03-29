#ifndef LLVM_BROWSE_DEFINITION_H
#define LLVM_BROWSE_DEFINITION_H

#include "Typedefs.h"

#include <llvm/Support/Casting.h>

namespace lb {

class INavigable;
class Module;

// This corresponds to a definition for a single entity in the IR.
// It contains the begin and end offsets of the definition within the IR
// as well as the entity to which that corresponds to that definition
class alignas(ALIGN_OBJ) Definition {
protected:
  uint64_t begin;
  uint64_t end;
  const INavigable* defined;

protected:
  Definition(uint64_t begin, uint64_t end, const INavigable& defined);

public:
  Definition()             = delete;
  Definition(Definition&)  = delete;
  Definition(Definition&&) = delete;
  virtual ~Definition()    = default;

  uint64_t get_begin() const;
  uint64_t get_end() const;
  const INavigable& get_defined() const;
  template<typename T>
  const T& get_defined_as() const {
    return *llvm::cast<T>(defined);
  }

  operator bool() const {
    return (begin > 0) and (end > 0);
  }

public:
  static Definition&
  make(uint64_t begin, uint64_t end, const INavigable& defined, Module& module);
};

} // namespace lb

#endif // LLVM_BROWSE_DEFINITION_H
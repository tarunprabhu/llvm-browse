#ifndef LLVM_BROWSE_SOURCE_RANGE_H
#define LLVM_BROWSE_SOURCE_RANGE_H

#include <string>

#include <llvm/ADT/StringRef.h>

#include "Typedefs.h"

namespace lb {

class Module;

class SourceRange {
protected:
  BufferId id;
  size_t begin;
  size_t end;

public:
  SourceRange();
  SourceRange(BufferId id, size_t begin, size_t end);
  virtual ~SourceRange() = default;

  bool is_valid() const;
  size_t get_begin() const;
  size_t get_end() const;
  llvm::StringRef get_text(const Module& module) const;
};

} // namespace lb

#endif // LLVM_BROWSE_SOURCE_RANGE_H

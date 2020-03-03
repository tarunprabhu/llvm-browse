#ifndef LLVM_BROWSE_SOURCE_RANGE_H
#define LLVM_BROWSE_SOURCE_RANGE_H

#include <string>

#include <llvm/ADT/StringRef.h>

#include "Typedefs.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

struct SourceRange {
  size_t begin;
  size_t end;

  SourceRange();
  SourceRange(size_t begin, size_t end);

  operator bool() const {
    return (begin > 0) and (end > 0);
  }
};

#ifdef __cplusplus
} // extern "C"
#endif // __cpluspls

#endif // LLVM_BROWSE_SOURCE_RANGE_H

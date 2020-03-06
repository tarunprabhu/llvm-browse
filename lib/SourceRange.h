#ifndef LLVM_BROWSE_SOURCE_RANGE_H
#define LLVM_BROWSE_SOURCE_RANGE_H

#include "Typedefs.h"

namespace lb {

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

struct SourcePoint {
  unsigned line;
  unsigned column;

  SourcePoint(unsigned line, unsigned column);

  operator bool() const {
    return (line > 0) and (column > 0);
  }
};

// Represents a range in the source file. This is separate from an LLVMRange
// because most of this information will come from LLVM's DI* objects
struct alignas(ALIGN_OBJ) SourceRange {
  // Full path to the source file. We can keep the pointer here because
  // if we do have a file, then it will be owned by one of the DI* objects
  // in the IR
  const char* file;
  SourcePoint begin;
  SourcePoint end;
  
  SourceRange();
  SourceRange(const char* file, unsigned begin_line, unsigned begin_col);
  SourceRange(const char* file,
              unsigned begin_line,
              unsigned begin_col,
              unsigned end_line,
              unsigned end_col);

  operator bool() const {
    // The end line and end column are optional. It is not always possible to
    // determine the range in the source, so we only need need the start
    return file and (begin.line > 0) and (begin.column > 0);
  }
};

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

} // namespace

#endif // LLVM_BROWSE_SOURCE_RANGE_H

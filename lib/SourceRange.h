#ifndef LLVM_BROWSE_SOURCE_RANGE_H
#define LLVM_BROWSE_SOURCE_RANGE_H

#include "SourcePoint.h"
#include "Typedefs.h"

namespace lb {

// Represents a range in the source file. This is separate from an LLVMRange
// because most of this information will come from LLVM's DI* objects
class alignas(ALIGN_OBJ) SourceRange {
protected:
  // Full path to the source file. We can keep the pointer here because
  // if we do have a file, then it will be owned by one of the DI* objects
  // in the IR
  const char* file;
  SourcePoint begin;
  SourcePoint end;

public:  
  SourceRange();
  SourceRange(const char* file, unsigned begin_line, unsigned begin_col);
  SourceRange(const char* file,
              unsigned begin_line,
              unsigned begin_col,
              unsigned end_line,
              unsigned end_col);

  const char* get_file() const;
  const SourcePoint& get_begin() const;
  unsigned get_begin_line() const;
  unsigned get_begin_column() const;
  const SourcePoint& get_end() const;
  unsigned get_end_line() const;
  unsigned get_end_column() const;

  operator bool() const {
    // The end line and end column are optional. It is not always possible to
    // determine the range in the source, so we only need need the start
    return file and (begin.get_line() > 0) and (begin.get_column() > 0);
  }
};

} // namespace

#endif // LLVM_BROWSE_SOURCE_RANGE_H

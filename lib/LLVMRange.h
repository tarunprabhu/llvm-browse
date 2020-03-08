#ifndef LLVM_BROWSE_LLVM_RANGE_H
#define LLVM_BROWSE_LLVM_RANGE_H

#include "Typedefs.h"

#include <stdint.h>

namespace lb {

// This represents a range in the LLVM IR. Here, everything is maintained
// as an offset into the file. The LLVM IR format is not guaranteed
// so it's hard enough and there doesn't seem to be a lot to be gained by
// keeping line and column numbers
class alignas(ALIGN_OBJ) LLVMRange {
protected:
  uint64_t begin;
  uint64_t end;

public:
  LLVMRange();
  LLVMRange(uint64_t begin, uint64_t end);

  uint64_t get_begin() const;
  uint64_t get_end() const;

  operator bool() const {
  	return (begin > 0) and (end > 0);
  }
};

} // namespace lb

#endif // LLVM_BROWSE_LLVM_RANGE_H
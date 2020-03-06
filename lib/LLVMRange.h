#ifndef LLVM_BROWSE_LLVM_RANGE_H
#define LLVM_BROWSE_LLVM_RANGE_H

#include "Typedefs.h"

#include <stdint.h>

namespace lb {

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// This represents a range in the LLVM IR. Here, everything is maintained
// as an offset into the file. The LLVM IR format is not guaranteed
// so it's hard enough and there doesn't seem to be a lot to be gained by
// keeping line and column numbers
struct alignas(ALIGN_OBJ) LLVMRange {
  uint64_t begin;
  uint64_t end;

  LLVMRange();
  LLVMRange(uint64_t begin, uint64_t end);

  operator bool() const {
  	return (begin > 0) and (end > 0);
  }
};

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

} // namespace lb

#endif // LLVM_BROWSE_LLVM_RANGE_H
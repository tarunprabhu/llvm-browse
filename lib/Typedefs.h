#ifndef LLVM_BROWSE_TYPEDEFS_H
#define LLVM_BROWSE_TYPEDEFS_H

#include <stdint.h>

namespace lb {

// Empty new line type used to align the output stream
struct NewLineT {};

// Not really a "type", but it only gets applied to type definitions
// The alignemnt guarantees that the lower-order bits of the
// pointer are zero. They can then be used to store tags
// when passing handles back and forth between C++ and Python
constexpr int ALIGN_OBJ = 16;

} // namespace lb

#endif // LLVM_BROWSE_TYPEDEFS_H

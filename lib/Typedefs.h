#ifndef LLVM_BROWSE_TYPEDEFS_H
#define LLVM_BROWSE_TYPEDEFS_H

#include <stdint.h>

namespace lb {

using BufferId = uint64_t;

// Empty new line type used to align the output stream 
struct NewLineT {
	;
};

} // namespace lb

#endif // LLVM_BROWSE_TYPEDEFS_H

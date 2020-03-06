#include "LLVMRange.h"

namespace lb {

LLVMRange::LLVMRange() : begin(0), end(0) {
  ;
}

LLVMRange::LLVMRange(uint64_t begin, uint64_t end) : begin(begin), end(end) {
  ;
}

} // namespace lb
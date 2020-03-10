#include "LLVMRange.h"

namespace lb {

LLVMRange::LLVMRange() : begin(0), end(0) {
  ;
}

LLVMRange::LLVMRange(Offset begin, Offset end) : begin(begin), end(end) {
  ;
}

Offset
LLVMRange::get_begin() const {
  return begin;
}

Offset
LLVMRange::get_end() const {
  return end;
}

} // namespace lb
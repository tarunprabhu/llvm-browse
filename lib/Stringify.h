#ifndef LLVM_BROWSE_STRINGIFY_H
#define LLVM_BROWSE_STRINGIFY_H

#include <llvm/Support/raw_ostream.h>

namespace lb {

template<typename T, std::enable_if_t<!std::is_pointer<T>::value, int> = 0>
std::string
str(const T& in) {
  std::string buf;
  llvm::raw_string_ostream ss(buf);
  ss << in;
  return ss.str();
}

} // namespace lb

#endif // LLVM_BROWSE_STRINGIFY_H

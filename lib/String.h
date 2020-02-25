#ifndef LLVM_BROWSE_STRING_H
#define LLVM_BROWSE_STRING_H

#include <llvm/Support/raw_ostream.h>

namespace lb {

namespace String {

  namespace impl {
    void concat(llvm::raw_string_ostream&);

    template<typename T, typename... ArgsT>
    void concat(llvm::raw_string_ostream& ss, T&& s, ArgsT&&... args) {
      ss << s;
      concat(ss, args...);
    }
  } // namespace impl

  template<typename T, std::enable_if_t<!std::is_pointer<T>::value, int> = 0>
  std::string str(const T& in) {
    std::string buf;
    llvm::raw_string_ostream ss(buf);
    ss << in;
    return ss.str();
  }

  template<typename... ArgsT>
  std::string concat(ArgsT&&... args) {
    std::string buf;
    llvm::raw_string_ostream ss(buf);
    impl::concat(ss, args...);
    return ss.str();
  }

} // namespace String

} // namespace lb

#endif // LLVM_BROWSE_STRING_H

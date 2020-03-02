#ifndef LLVM_BROWSE_LOGGING_H
#define LLVM_BROWSE_LOGGING_H

#include "Typedefs.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/FormattedStream.h>

namespace lb {

class FormattedStream final {
protected:
  llvm::formatted_raw_ostream fs;
  const std::string label;
  llvm::raw_ostream::Colors color;

protected:
  void pad();

public:
  FormattedStream(const std::string& label, llvm::raw_ostream::Colors color);
  virtual ~FormattedStream() = default;

  template<typename T>
  FormattedStream& operator<<(const T& inp) {
    fs << inp;
    return *this;
  }

  FormattedStream& operator<<(const NewLineT&);
  FormattedStream& start();
};

FormattedStream&
message(bool start = true);
FormattedStream&
warning(bool start = true);
FormattedStream&
critical(bool start = true);
FormattedStream&
error(bool start = true);

extern const NewLineT endl;

} // namespace lb

#endif // LLVM_BROWSE_LOGGING_H
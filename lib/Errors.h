#ifndef LLVM_BROWSE_ERRORS_H
#define LLVM_BROWSE_ERRORS_H

struct ErrorCode {
  enum {
    None = 0,
    CommandLineArg,
    ExtraCommandLineArg,
    ModuleLoad,
  } value;

  ErrorCode(decltype(value) val) : value(val) {
    ;
  }

  explicit operator bool() {
    return value != ErrorCode::None;
  }

  operator int() {
    return static_cast<int>(value);
  }
};

#endif // LLVM_BROWSE_ERRORS_H

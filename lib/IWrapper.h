#ifndef LLVM_BROWSE_IWRAPPER_H
#define LLVM_BROWSE_IWRAPPER_H

namespace lb {

class Module;

// Yes, it's an interface and really shouldn't have any data items ...
template<typename LLVM_T>
class IWrapper {
public:
  typedef std::conditional_t<std::is_pointer<LLVM_T>::value, LLVM_T, LLVM_T&>
      WrappedType;

private:
  WrappedType llvm_t;
  Module& module;

protected:
  IWrapper()           = delete;
  IWrapper(IWrapper&)  = delete;
  IWrapper(IWrapper&&) = delete;
  IWrapper(WrappedType llvm_t, Module& module) :
      llvm_t(llvm_t), module(module) {
    ;
  }

  Module& get_module() {
    return module;
  }

public:
  virtual ~IWrapper() = default;

  const Module& get_module() const {
    return module;
  }

  const WrappedType get_llvm() const {
    return llvm_t;
  }
};

} // namespace lb

#endif // LLVM_BROWSE_IWRAPPER_H

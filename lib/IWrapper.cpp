#include "IWrapper.h"

namespace lb {

IWrapper::IWrapper(Module& module) : module(module) {
  ;
}

Module&
IWrapper::get_module() {
  return module;
}

const Module&
IWrapper::get_module() const {
  return module;
}

} // namespace lb

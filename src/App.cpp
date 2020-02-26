#include "App.h"

namespace lb {

bool
App::open_file(const std::string& file) {
  context.reset(new llvm::LLVMContext());
  module = std::move(Module::create(file, *context));
  if(not module)
    return false;
  return true;
}

const Module&
App::get_module() const {
  return *module;
}

} // namespace lb

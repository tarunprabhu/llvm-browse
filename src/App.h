#ifndef LLVM_BROWSE_APP_H
#define LLVM_BROWSE_APP_H

#include <llvm/IR/LLVMContext.h>

#include "lib/Module.h"

#include <string>

namespace lb {

class App {
protected:
  std::unique_ptr<llvm::LLVMContext> context;
  std::unique_ptr<Module> module;

public:
  App() = default;
  App(App&) = delete;
  App(App&&) = delete;
  virtual ~App() = default;

  bool open_file(const std::string& file);

  const Module& get_module() const;
};

} // namespace lb

#endif // LLVM_BROWSE_APP_H

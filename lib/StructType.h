#ifndef LLVM_BROWSE_STRUCT_TYPE_H
#define LLVM_BROWSE_STRUCT_TYPE_H

#include <llvm/IR/DerivedTypes.h>

#include "Navigable.h"

namespace lb {

class Module;

class StructType : public Navigable {
protected:
  // For reasons that I ought to remember but can't, declaring LLVM types as
  // const causes problems (I think it's with the DataLayout objects that we
  // probably will never use here, but still). Also, references to types seem
  // odd, but that might just be because you almost never see them anywhere
  // in LLVM's APIs. So the types remain non-const. 
  llvm::StructType* llvm;
  Module& module;

public:
  StructType(llvm::StructType* llvm, Module& module);
  virtual ~StructType() = default;

  llvm::StructType* get_llvm() const;
  const Module& get_module() const;
};

} // namespace lb

#endif // LLVM_BROWSE_STRUCT_TYPE_H

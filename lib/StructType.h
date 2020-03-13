#ifndef LLVM_BROWSE_STRUCT_TYPE_H
#define LLVM_BROWSE_STRUCT_TYPE_H

#include <llvm/IR/DerivedTypes.h>

#include "INavigable.h"
#include "IWrapper.h"
#include "Typedefs.h"

namespace lb {

class Module;

class alignas(ALIGN_OBJ) StructType :
    public INavigable,
    public IWrapper<llvm::StructType*> {
protected:
  // For reasons that I ought to remember but can't, declaring LLVM types as
  // const causes problems (I think it's with the DataLayout objects that we
  // probably will never use here, but still). Also, references to types seem
  // odd, but that might just be because you almost never see them anywhere
  // in LLVM's APIs. So the types remain non-const.
  llvm::StructType* llvm;
  std::string source_name;
  std::string full_name;
  std::string qualified_name;

protected:
  StructType(llvm::StructType* llvm, Module& module);

public:
  StructType()             = delete;
  StructType(StructType&)  = delete;
  StructType(StructType&&) = delete;
  virtual ~StructType()    = default;

  bool has_source_info() const;
  bool has_source_name() const;
  bool has_full_name() const;
  bool has_qualified_name() const;
  llvm::StringRef get_source_name() const;
  llvm::StringRef get_llvm_name() const;
  llvm::StringRef get_full_name() const;
  llvm::StringRef get_qualified_name() const;
  bool is_artificial() const;

public:
  static bool classof(const INavigable* v) {
    return v->get_kind() == EntityKind::StructType;
  }

  static StructType& make(llvm::StructType* sty, Module& module);
};

} // namespace lb

#endif // LLVM_BROWSE_STRUCT_TYPE_H

#include "StructType.h"
#include "Logging.h"
#include "Module.h"

#include <llvm/Support/raw_ostream.h>

namespace lb {

StructType::StructType(llvm::StructType* llvm, Module& module) :
    INavigable(EntityKind::StructType),
    IWrapper<llvm::StructType*>(llvm, module) {

  if(llvm->hasName())
    set_tag(llvm->getName(), "%");
  else
    critical() << "Cannot set tag for unnamed struct: " << *get_llvm() << "\n";

  // TODO: Get the source name and the full name from the DebugInfo
}

bool
StructType::has_source_info() const {
  // It is not straightforward to match up the source information to the
  // LLVM struct types. In the case of C++, the types themselves are likely
  // to be broken up into pieces (in the case of multiple inheritance), or
  // have extra fields that don't match up with the source (because of the
  // insertion of vtables). Still, there could still be a reasonable
  // attempt at getting there, so eventually, it should be implemented
  return false;
}

bool
StructType::has_source_name() const {
  return get_source_name().size();
}

llvm::StringRef
StructType::get_source_name() const {
  return llvm::StringRef(source_name);
}

llvm::StringRef
StructType::get_llvm_name() const {
  return get_llvm()->getName();
}

llvm::StringRef
StructType::get_full_name() const {
  return llvm::StringRef(full_name);
}

bool
StructType::is_artificial() const {
  // At some point, we may be able to make a distinction between struct types
  // that are defined by the user (or inherited from other header files) and
  // those that are a result of the compiler creating a struct. Off the top
  // of my head, I can't think of a case where the latter is true anyway,
  // but still, it's something to keep in mind
  return false;
}

StructType&
StructType::make(llvm::StructType* llvm_sty, Module& module) {
  auto* sty = new StructType(llvm_sty, module);
  module.m_structs.emplace_back(sty);
  module.tmap[llvm_sty] = sty;

  return *sty;
}

} // namespace lb

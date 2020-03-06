#include "StructType.h"
#include "Logging.h"

#include <llvm/Support/raw_ostream.h>

namespace lb {

StructType::StructType(llvm::StructType* llvm, Module& module) :
    INavigable(), IWrapper<llvm::StructType*>(llvm, module) {
  if(llvm->hasName())
    set_tag(llvm->getName(), "%");
  else
    critical() << "Cannot set tag for unnamed struct: " << *get_llvm() << "\n";

  // TODO: Get the source name and the full name from the DebugInfo
}

bool StructType::has_source_info() const {
	// It is not straightforward to match up the source information to the
	// LLVM struct types. In the case of C++, the types themselves are likely
	// to be broken up into pieces (in the case of multiple inheritance), or
	// have extra fields that don't match up with the source (because of the
	// insertion of vtables). Still, there could still be a reasonable
	// attempt at getting there, so eventually, it should be implemented
	return false;
}

bool StructType::has_source_name() const {
	return get_source_name().size();
}

llvm::StringRef StructType::get_source_name() const {
	return llvm::StringRef(source_name);
}

llvm::StringRef StructType::get_llvm_name() const {
	return get_llvm()->getName();
}

llvm::StringRef StructType::get_full_name() const {
	return llvm::StringRef(full_name);
}

} // namespace lb

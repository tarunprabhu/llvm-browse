#ifndef LLVM_BROWSE_DI_UTILS_H
#define LLVM_BROWSE_DI_UTILS_H

#include <llvm/IR/DebugInfoMetadata.h>

#include <string>

namespace lb {

namespace DebugInfo {

std::string
get_name(const llvm::DINamespace* di, bool keep_templates=true);
std::string
get_name(const llvm::DICompositeType*, bool keep_templates=true);
std::string
get_name(const llvm::DILocalVariable*, bool keep_templates=true);
std::string
get_name(const llvm::DIGlobalVariable*, bool keep_templates=true);
std::string
get_name(const llvm::DISubprogram*, bool keep_templates=true);

std::string
get_full_name(const llvm::DISubprogram*);
std::string
get_full_name(const llvm::DIGlobalVariable*);

std::string
get_qualified_name(const llvm::DISubprogram*);
std::string
get_qualified_name(const llvm::DIGlobalVariable*);

} // namespace DebugInfo

} // namespace lb

#endif // LLVM_BROWSE_DI_UTILS_H
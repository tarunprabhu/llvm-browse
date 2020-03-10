#ifndef LLVM_BROWSE_DI_UTILS_H
#define LLVM_BROWSE_DI_UTILS_H

#include <llvm/IR/DebugInfoMetadata.h>

#include <string>

namespace lb {

namespace DebugInfo {

std::string
get_name(const llvm::DINamespace*);
std::string
get_name(const llvm::DICompositeType*);
std::string
get_name(const llvm::DILocalVariable*);
std::string
get_name(const llvm::DIGlobalVariable*);
std::string
get_name(const llvm::DISubprogram*);

std::string
get_full_name(const llvm::DISubprogram*);
std::string
get_full_name(const llvm::DIGlobalVariable*);

} // namespace DebugInfo

} // namespace lb

#endif // LLVM_BROWSE_DI_UTILS_H
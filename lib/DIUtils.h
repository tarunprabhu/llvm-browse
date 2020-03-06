#ifndef LLVM_BROWSE_DI_UTILS_H
#define LLVM_BROWSE_DI_UTILS_H

#include <llvm/IR/DebugInfoMetadata.h>

namespace lb {

namespace DebugInfo {

std::string get_full_name(const llvm::DISubprogram*);
std::string get_full_name(const llvm::DIGlobalVariable*);

} // DebugInfo

} // namespace lb

#endif // LLVM_BROWSE_DI_UTILS_H
#include "SourceRange.h"
#include "Module.h"

SourceRange::SourceRange() :
    begin(llvm::StringRef::npos), end(llvm::StringRef::npos) {
  ;
}

SourceRange::SourceRange(size_t begin, size_t end) :
    begin(begin), end(end) {
  ;
}

// bool
// SourceRange::is_valid() const {
//   return (begin != llvm::StringRef::npos) and (end != llvm::StringRef::npos);
// }

#include "SourceRange.h"
#include "Module.h"

namespace lb {

SourceRange::SourceRange() :
    id(Module::get_invalid_id()), begin(llvm::StringRef::npos),
    end(llvm::StringRef::npos) {
  ;
}

SourceRange::SourceRange(size_t begin, size_t end) :
    id(Module::get_main_id()), begin(begin), end(end) {
  ;
}

SourceRange::SourceRange(BufferId id, size_t begin, size_t end) :
    id(id), begin(begin), end(end) {
  ;
}

bool
SourceRange::is_valid() const {
  return (id != Module::get_invalid_id()) and (begin != llvm::StringRef::npos)
         and (end != llvm::StringRef::npos);
}

BufferId
SourceRange::get_id() const {
  return id;
}

size_t
SourceRange::get_begin() const {
  return begin;
}

size_t
SourceRange::get_end() const {
  return end;
}

llvm::StringRef
SourceRange::get_text(const Module& module) const {
  return module.get_contents(id).substr(begin, end - begin);
}

} // namespace lb

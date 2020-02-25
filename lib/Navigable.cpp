#include "Navigable.h"
#include "String.h"

namespace lb {

static bool
needs_quotes(llvm::StringRef str) {
  for(char c : str)
    if(not(std::isalnum(c) or (c == '.') or (c == '_')))
      return true;
  return false;
}

void
Navigable::set_tag(int slot) {
  tag = String::concat("%", slot);
}

void
Navigable::set_tag(llvm::StringRef name,
                   llvm::StringRef prefix,
                   bool may_need_quotes) {
  if(may_need_quotes and needs_quotes(name))
    tag = String::concat(prefix, "\"", name, "\"");
  else
    tag = String::concat(prefix, name);
}

void
Navigable::set_llvm_range(const SourceRange& range) {
  llvm = range;
}

void
Navigable::set_source_range(const SourceRange& range) {
  source = range;
}

llvm::StringRef
Navigable::get_tag() const {
  return llvm::StringRef(tag);
}

const SourceRange&
Navigable::get_llvm_range() const {
  return llvm;
}

const SourceRange&
Navigable::get_source_range() const {
  return source;
}

} // namespace lb

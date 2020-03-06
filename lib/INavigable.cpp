#include "INavigable.h"
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
INavigable::sort_uses() {
  std::sort(m_uses.begin(),
            m_uses.end(),
            [](const LLVMRange& l, const LLVMRange& r) {
              return ((l.begin < r.begin) and (l.end < r.end));
            });
}

void
INavigable::add_use(const LLVMRange& range) {
  m_uses.emplace_back(range);
}

void
INavigable::set_tag(unsigned slot, llvm::StringRef prefix) {
  tag = String::concat(prefix, slot);
}

void
INavigable::set_tag(llvm::StringRef name) {
  tag = name;
}

void
INavigable::set_tag(llvm::StringRef name,
                    llvm::StringRef prefix,
                    bool may_need_quotes) {
  if(may_need_quotes and needs_quotes(name))
    tag = String::concat(prefix, "\"", name, "\"");
  else
    tag = String::concat(prefix, name);
}

void
INavigable::set_llvm_defn(const LLVMRange& range) {
  llvm_defn = range;
}

void
INavigable::set_llvm_span(const LLVMRange& range) {
  llvm_span = range;
}

void
INavigable::set_source_defn(const SourceRange& range) {
  source_defn = range;
}

void
INavigable::set_source_span(const SourceRange& range) {
  source_span = range;
}

bool
INavigable::has_tag() const {
  return tag.length();
}

llvm::StringRef
INavigable::get_tag() const {
  return llvm::StringRef(tag);
}

INavigable::Iterator
INavigable::begin() const {
  return m_uses.cbegin();
}

INavigable::Iterator
INavigable::end() const {
  return m_uses.cend();
}

llvm::iterator_range<INavigable::Iterator>
INavigable::uses() const {
  return llvm::iterator_range<Iterator>(m_uses);
}

unsigned
INavigable::get_num_uses() const {
  return m_uses.size();
}

bool INavigable::has_llvm_defn() const {
  return llvm_defn;
}

bool INavigable::has_llvm_span() const {
  return llvm_span;
}

bool INavigable::has_source_defn() const {
  return source_defn;
}

bool INavigable::has_source_span() const {
  return source_span;
}

const LLVMRange&
INavigable::get_llvm_defn() const {
  return llvm_defn;
}

const LLVMRange&
INavigable::get_llvm_span() const {
  return llvm_span;
}

const SourceRange&
INavigable::get_source_defn() const {
  return source_defn;
}
const SourceRange&
INavigable::get_source_span() const {
  return source_span;
}

} // namespace lb
  
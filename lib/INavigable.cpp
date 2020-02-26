#include "INavigable.h"
#include "String.h"

#include <glib.h>

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
            [](const SourceRange& l, const SourceRange& r) {
              return (l.get_id() < r.get_id())
                     and (l.get_begin() < r.get_begin())
                     and (l.get_end() < r.get_end());
            });
}

void
INavigable::add_use(const SourceRange& range) {
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
INavigable::set_defn_range(const SourceRange& range) {
  defn = range;
}

void
INavigable::set_llvm_range(const SourceRange& range) {
  llvm = range;
}

void
INavigable::set_source_range(const SourceRange& range) {
  source = range;
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

const SourceRange&
INavigable::get_defn_range() const {
  return defn;
}

const SourceRange&
INavigable::get_llvm_range() const {
  return llvm;
}

const SourceRange&
INavigable::get_source_range() const {
  return source;
}

} // namespace lb

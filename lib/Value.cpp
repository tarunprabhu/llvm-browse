#include "Value.h"
#include "Module.h"

namespace lb {

Value::Value(Value::Kind kind, const llvm::Value& llvm, Module& module) :
    Navigable(), module(module), llvm(llvm), kind(kind) {
  ;
}

void
Value::sort_uses() {
  std::sort(m_uses.begin(),
            m_uses.end(),
            [](const SourceRange& l, const SourceRange& r) {
              return (l.get_id() < r.get_id())
                     and (l.get_begin() < r.get_begin())
                     and (l.get_end() < r.get_end());
            });
}

void
Value::add_use(const SourceRange& range) {
  m_uses.emplace_back(range);
}

Value::Iterator
Value::begin() const {
  return m_uses.begin();
}

Value::Iterator
Value::end() const {
  return m_uses.end();
}

llvm::iterator_range<Value::Iterator>
Value::uses() const {
  return llvm::iterator_range<Value::Iterator>(m_uses);
}

unsigned
Value::get_num_uses() const {
  return m_uses.size();
}

Value::Kind
Value::get_kind() const {
  return kind;
}

const Module&
Value::get_module() const {
  return module;
}

} // namespace lb

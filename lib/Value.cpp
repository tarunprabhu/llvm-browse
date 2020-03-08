#include "Value.h"
#include "Module.h"

namespace lb {

Value::Value(EntityKind kind) : kind(kind) {
  ;
}

EntityKind
Value::get_kind() const {
  return kind;
}

} // namespace lb

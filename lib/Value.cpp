#include "Value.h"
#include "Module.h"

namespace lb {

Value::Value(Value::Kind kind) :
    kind(kind) {
  ;
}

Value::Kind
Value::get_kind() const {
  return kind;
}

} // namespace lb

#include "Definition.h"
#include "Module.h"

namespace lb {

Definition::Definition(uint64_t begin,
                       uint64_t end,
                       const INavigable& defined) :
    begin(begin), end(end), defined(&defined) {
  ;
}

uint64_t
Definition::get_begin() const {
  return begin;
}

uint64_t
Definition::get_end() const {
  return end;
}

const INavigable*
Definition::get_defined() const {
  return defined;
}

Definition&
Definition::make(uint64_t begin,
                 uint64_t end,
                 const INavigable& defined,
                 Module& module) {
  auto* def = new Definition(begin, end, defined);
  module.defs.emplace_back(def);

  return *def;
}

} // namespace lb
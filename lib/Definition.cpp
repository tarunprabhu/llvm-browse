#include "Definition.h"

namespace lb {

Definition::Definition(uint64_t begin,
                       uint64_t end,
                       const INavigable& defined) :
    begin(begin),
    end(end), defined(&defined) {
  ;
}

uint64_t Definition::get_begin() const {
	return begin;
}

uint64_t Definition::get_end() const {
	return end;
}

const INavigable* Definition::get_defined() const {
	return defined;
}

} // namespace lb
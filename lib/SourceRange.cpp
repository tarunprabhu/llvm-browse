#include "SourceRange.h"
#include "SourceFile.h"

namespace lb {

// SourceRange::SourceRange(const SourceFile& file) :
//     file(file), begin(std::make_tuple(0, 0)), end(std::make_tuple(0, 0)) {
//   ;
// }

SourceRange::SourceRange(const SourceFile& file,
                         const std::tuple<unsigned, unsigned>& begin,
                         const std::tuple<unsigned, unsigned>& end) :
    file(&file),
    begin(begin), end(end) {
  ;
}

const SourceFile&
SourceRange::get_file() const {
  return *file;
}

unsigned
SourceRange::get_begin_line() const {
  return std::get<0>(begin);
}

unsigned
SourceRange::get_begin_col() const {
  return std::get<1>(begin);
}

unsigned
SourceRange::get_end_line() const {
  return std::get<0>(end);
}

unsigned
SourceRange::get_end_col() const {
  return std::get<1>(end);
}

const std::tuple<unsigned, unsigned>&
SourceRange::get_begin() const {
  return begin;
}

const std::tuple<unsigned, unsigned>&
SourceRange::get_end() const {
  return end;
}

} // namespace lb

#include "SourceRange.h"
#include "SourceFile.h"

namespace lb {

SourceRange::SourceRange(const SourceFile& file, uint64_t begin, uint64_t end) :
    file(&file), begin(begin), end(end) {
  ;
}

const SourceFile&
SourceRange::get_file() const {
  return *file;
}

uint64_t
SourceRange::get_begin() const {
  return begin;
}

uint64_t
SourceRange::get_end() const {
  return end;
}

std::string
SourceRange::get_text() const {
  return file->get_contents().substr(begin, end - begin);
}

} // namespace lb

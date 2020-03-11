#include "SourceRange.h"
#include "Module.h"

namespace lb {

SourceRange::SourceRange() : file(nullptr), begin(0, 0), end(0, 0) {
  ;
}

SourceRange::SourceRange(llvm::StringRef file, unsigned line, unsigned column) :
    file(file.data()), begin(line, column), end(0, 0) {
  ;
}

SourceRange::SourceRange(llvm::StringRef file,
                         unsigned begin_line,
                         unsigned begin_column,
                         unsigned end_line,
                         unsigned end_column) :
    file(file.data()),
    begin({begin_line, begin_column}),
    end({end_line, end_column}) {
  ;
}

const char*
SourceRange::get_file() const {
  return file;
}

const SourcePoint&
SourceRange::get_begin() const {
  return begin;
}

unsigned
SourceRange::get_begin_line() const {
  return get_begin().get_line();
}

unsigned
SourceRange::get_begin_column() const {
  return get_begin().get_column();
}

const SourcePoint&
SourceRange::get_end() const {
  return end;
}

unsigned
SourceRange::get_end_line() const {
  return get_end().get_line();
}

unsigned
SourceRange::get_end_column() const {
  return get_end().get_column();
}

} // namespace lb
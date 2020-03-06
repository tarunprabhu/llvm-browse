#include "SourceRange.h"
#include "Module.h"

namespace lb {

SourcePoint::SourcePoint(unsigned line, unsigned column) :
    line(line), column(column) {
  ;
}

SourceRange::SourceRange() : file(nullptr), begin(0, 0), end(0, 0) {
  ;
}

SourceRange::SourceRange(const char* file, unsigned line, unsigned column) :
    file(file), begin(line, column), end(0, 0) {
  ;
}

SourceRange::SourceRange(const char* file,
                         unsigned begin_line,
                         unsigned begin_column,
                         unsigned end_line,
                         unsigned end_column) :
    file(file),
    begin({begin_line, begin_column}), end({end_line, end_column}) {
  ;
}

} // namespace lb
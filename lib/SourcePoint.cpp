#include "SourcePoint.h"

namespace lb {

SourcePoint::SourcePoint(unsigned line, unsigned column) :
    line(line), column(column) {
  ;
}

unsigned
SourcePoint::get_line() const {
  return line;
}

unsigned
SourcePoint::get_column() const {
  return column;
}

} // namespace lb
#ifndef LLVM_BROWSE_SOURCE_POINT_H
#define LLVM_BROWSE_SOURCE_POINT_H

#include "Typedefs.h"

namespace lb {

class alignas(ALIGN_OBJ) SourcePoint {
protected:
  unsigned line;
  unsigned column;

public:
  SourcePoint(unsigned line, unsigned column);

  unsigned get_line() const;
  unsigned get_column() const;

  operator bool() const {
    return (line > 0) and (column > 0);
  }
};

} // namespace lb

#endif // LLVM_BROWSE_SOURCE_POINT_H
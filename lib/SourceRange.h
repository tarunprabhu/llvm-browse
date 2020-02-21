#ifndef LLVM_BROWSE_SOURCE_RANGE_H
#define LLVM_BROWSE_SOURCE_RANGE_H

#include <tuple>

namespace lb {

class SourceFile;

class SourceRange {
protected:
  const SourceFile* file;
  std::tuple<unsigned, unsigned> begin;
  std::tuple<unsigned, unsigned> end;

public:
  SourceRange() = default;
  SourceRange(const SourceFile&,
              const std::tuple<unsigned, unsigned>&,
              const std::tuple<unsigned, unsigned>&);
  virtual ~SourceRange() = default;

  const SourceFile& get_file() const;
  const std::tuple<unsigned, unsigned>& get_begin() const;
  const std::tuple<unsigned, unsigned>& get_end() const;

  unsigned get_begin_line() const;
  unsigned get_begin_col() const;
  unsigned get_end_line() const;
  unsigned get_end_col() const;
};

} // namespace lb

#endif // LLVM_BROWSE_SOURCE_RANGE_H

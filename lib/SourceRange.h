#ifndef LLVM_BROWSE_SOURCE_RANGE_H
#define LLVM_BROWSE_SOURCE_RANGE_H

#include <stdint.h>
#include <string>

namespace lb {

class SourceFile;

class SourceRange {
protected:
  const SourceFile* file;
  uint64_t begin;
  uint64_t end;

public:
  SourceRange() = default;
  SourceRange(const SourceFile& file, uint64_t begin, uint64_t end);
  virtual ~SourceRange() = default;

  const SourceFile& get_file() const;
  uint64_t get_begin() const;
  uint64_t get_end() const;
  std::string get_text() const;
};

} // namespace lb

#endif // LLVM_BROWSE_SOURCE_RANGE_H

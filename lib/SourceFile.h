#ifndef LLVM_BROWSE_SOURCE_FILE_H
#define LLVM_BROWSE_SOURCE_FILE_H

#include <string>

namespace lb {

class SourceFile {
protected:
  std::string path;
  std::string contents;

public:
  SourceFile() = default;
  SourceFile(const std::string&);
  virtual ~SourceFile() = default;

  void set_contents(const std::string&);

  bool is_valid() const;
  bool is_llvm() const;
  const std::string& get_path() const;
  const std::string& get_contents() const;
};

} // namespace lb

#endif // LLVM_BROWSE_SOURCE_FILE_H

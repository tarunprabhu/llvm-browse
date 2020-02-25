#ifndef LLVM_BROWSE_SOURCE_FILE_H
#define LLVM_BROWSE_SOURCE_FILE_H

#include <llvm/Support/MemoryBuffer.h>

#include <memory>
#include <string>

namespace lb {

class SourceFile {
protected:
  std::string path;
  std::unique_ptr<llvm::MemoryBuffer> mbuf;

public:
  SourceFile() = default;
  SourceFile(const std::string& path);
  virtual ~SourceFile() = default;

  void set_contents(std::unique_ptr<llvm::MemoryBuffer>& mbuf);
  
  operator bool() {
    return mbuf->getBuffer().size();
  }
  bool is_valid() const;
  bool is_llvm() const;
  const std::string& get_path() const;
  const std::string& get_contents() const;
};

} // namespace lb

#endif // LLVM_BROWSE_SOURCE_FILE_H

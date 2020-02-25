#include "SourceFile.h"

#include <fstream>

namespace lb {

SourceFile::SourceFile(const std::string& path) : path(path) {
  // std::ifstream in(path, std::ios_base::in | std::ios_base::ate);
  // uint64_t size = in.tellg();
  // contents.reserve(size);
  // in.seekg(0);
  // in.read(&contents[0], size);
}

void
SourceFile::set_contents(std::unique_ptr<llvm::MemoryBuffer>& mbuf) {
  this->mbuf = std::move(mbuf);
}

bool
SourceFile::is_valid() const {
  return mbuf->getBuffer().size();
}

bool
SourceFile::is_llvm() const {
  return is_valid() and (not path.length());
}

const std::string&
SourceFile::get_path() const {
  return path;
}

const std::string&
SourceFile::get_contents() const {
  return mbuf->getBuffer().str();
}

} // namespace lb

#ifndef LLVM_BROWSE_PARSER_H
#define LLVM_BROWSE_PARSER_H

#include <llvm/AsmParser/SlotMapping.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/MemoryBuffer.h>

#include <memory>
#include <set>

namespace lb {

class Module;

// Currently, this is not actually a parser, but it really ought to be
// because that would be far more efficient than the current kludgy way of
// doing things.
//
// The parser as it currently is a collection of string search functions
// that gets used to associate locations in the IR with the various LLVM
// entities. What this parser does is keep track of a "cursor" position in
// the IR. As each entity is parsed, the cursor position is adjusted so all
// the nasty string searches that we do are as efficient as possible.
//
class Parser {
protected:
  std::unique_ptr<llvm::ModuleSlotTracker> local_slots;
  std::unique_ptr<llvm::SlotMapping> global_slots;
  llvm::StringRef ir;

protected:
  size_t find(llvm::StringRef key,
              bool at_start,
              size_t cursor,
              bool wrap,
              std::set<size_t>& seen);
  size_t find(llvm::StringRef key, bool at_start, size_t cursor, bool wrap);
  
  // Find the key in the IR. Start searching from the current location of the
  // cursor and wrap around if necessary. The cursor will be positioned at
  // the end of first instance of key found in the IR. It is an error if the
  // key is not found. Returns the offset of the first instance of key
  //
  // The idea here is that the entities returned from the llvm::Module
  // (llvm::StructType, llvm::GlobalVariable, llvm::Function) will be returned
  // in the order in which they occur in the IR. In that case, using the
  // cursor as the starting point of the search should reduce the amount of
  // time spent searching but in the worst case, everything still should
  // be found. The key should be chosen so that it doesn't return more than
  // one match as much as possible. If at_start is true, the key is expected
  // at the start of a line
  //
  size_t find_and_move(const std::string& key, bool at_start, size_t& cursor);
  size_t find_and_move(llvm::StringRef key, bool at_start, size_t& cursor);
  size_t find_function(llvm::StringRef func,
                       llvm::StringRef prefix,
                       size_t& cursor,
                       std::set<size_t>& seen);
  size_t
  find_function(llvm::StringRef func, llvm::StringRef prefix, size_t& cursor);

public:
  Parser();
  virtual ~Parser() = default;

  std::tuple<std::unique_ptr<llvm::Module>, std::unique_ptr<llvm::MemoryBuffer>>
  parse_ir(std::unique_ptr<llvm::MemoryBuffer> in, llvm::LLVMContext& context);
  std::tuple<std::unique_ptr<llvm::Module>, std::unique_ptr<llvm::MemoryBuffer>>
  parse_bc(std::unique_ptr<llvm::MemoryBuffer> in, llvm::LLVMContext& context);

  // Associate the entities in the module with appropriate line numbers and
  // ranges in the text representation of the IR
  bool link(Module&);
  llvm::SlotMapping& get_local_slots();
  llvm::ModuleSlotTracker get_global_slots();
};

} // namespace lb

#endif // LLVM_BROWSE_PARSER_H

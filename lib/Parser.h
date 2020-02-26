#ifndef LLVM_BROWSE_PARSER_H
#define LLVM_BROWSE_PARSER_H

#include <llvm/AsmParser/SlotMapping.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/MemoryBuffer.h>

#include <map>
#include <memory>
#include <set>

namespace lb {

class Module;
class INavigable;
class Value;

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
  // When determining the locations for the various entities, we need to look
  // at the previous character to make sure that we don't have any false
  // matches
  enum class Lookback {
    // Doesn't matter what the previous char is
    Any,

    // The previous character must be a newline because the string has to match
    // at the start of a line
    Newline,

    // Whitespace. This subsumes Newline
    Whitespace,

    // There should be one or more whitespaces until a newline with
    // no intervening non-whitespace characters
    Indent,
  };

protected:
  std::unique_ptr<llvm::ModuleSlotTracker> local_slots;
  std::unique_ptr<llvm::SlotMapping> global_slots;
  llvm::StringRef ir;

protected:
  std::vector<const llvm::MDNode*> get_metadata(const llvm::GlobalObject&);
  std::vector<const llvm::MDNode*> get_metadata(const llvm::Instruction&);
  
  // Helper function used when assocating values with uses.
  // Checks if the position has already been associated with another value
  // in the map
  bool overlaps(size_t pos, const std::map<INavigable*, size_t>& mapped);

  // Associate the values with uses starting at the cursor
  std::map<INavigable*, size_t>
  associate_values(std::vector<INavigable*> values, size_t cursor);

  size_t find(llvm::StringRef key,
              size_t cursor,
              Lookback prev,
              bool wrap,
              std::set<size_t>& seen);
  size_t find(llvm::StringRef key, size_t cursor, Lookback prev, bool wrap);

  // This is meant to find the operands of a constant (typically a
  // ConstantArray, ConstantStruct or ConstantExpr) that we want to be able
  // to navigate to. Typically, we are only concerned with the top-level
  // entities like functions or globals, but we expand it to include the
  // global indirect objects as well in case we ever support it.
  // This will be called when we encounter a ConstantExpr in an instruction
  // or in the initializer of a global variable. We return a vector instead of
  // a set because there may be multiple occurences of the same global value
  // in the operands of the constant and we want to be able to find them all
  // later so we can hook them up, so duplicates in this case are very much
  // desired
  void collect_constants(const llvm::Constant* c,
                         Module& module,
                         std::vector<INavigable*>& consts);
  std::vector<INavigable*> collect_constants(const llvm::Constant* c,
                                             Module& module);

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
  size_t find_and_move(const std::string& key, Lookback prev, size_t& cursor);
  size_t find_and_move(llvm::StringRef key, Lookback prev, size_t& cursor);
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
};

} // namespace lb

#endif // LLVM_BROWSE_PARSER_H

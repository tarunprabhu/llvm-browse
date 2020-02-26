#ifndef LLVM_BROWSE_NAVIGABLE_H
#define LLVM_BROWSE_NAVIGABLE_H

#include <string>

#include <llvm/ADT/StringRef.h>

#include "SourceRange.h"

namespace lb {

// Base for objects that are navigable. This essentially means that they
// have a location in the LLVM file that can be reached with a
// "goto-definition" command and may also have a location in the source-file
// associated with them. They also have a handle which is a text string
// used in the LLVM IR to uniquely identify them (typically matching either of
// the regexes R"^@.+$" or R"^%.+$". We specifically don't
// include uses here because StructType's are also navigable and it makes
// sense to go to the definition of a struct type but it doesn't really
// make sense to find a "use" of it. An instantiation of a struct would be
// a different story, but that is out of the scope of this interface.
// The source range is optional because not all entities will have a
// corresponding location in the source. Examples of these would be
// vtables and typeinfo objects
//
class Navigable {
protected:
  std::string tag;

  // The defn range is the range of characters in the IR that correspond to
  // the "definition" of an entity. The "definition" is where the cursor must
  // go when the user asks to go to the definition of an entity.
  // For basic blocks, it corresponds to the start of the first instruction
  // in the block and may be of length 0. For instructions that return a value,
  // this will be the "%<slot> = % part of the instruction while for those 
  // that don't return a value, it may cover the instruction opcode although
  // it might be better if it is of length 0 and positioned just at the start
  // of the op code
  SourceRange defn;

  // The LLVM range is the range in characters that the entity covers in
  // the IR. For functions, this is the entire body, for basic blocks, all
  // the characters from the start of the first instruction in the block to
  // the end of the last. It may be invalid for other entities
  SourceRange llvm;

  // The source range is the range in characters in the source code that the
  // entity covers. This is a somewhat more nebulous range because there may not
  // be a reasonable mapping from the source to LLVM. For instance, multiple
  // instructions could map to the same lines in the source code in which case
  // the source range of all those instructions are likely to be identical
  // Still, this is mainly here so we have a decent starting point at which
  // to position the cursor in the source even if we can't do anything else
  // beyond that 
  SourceRange source;

protected:
  Navigable() = default;


public:
  virtual ~Navigable() = default;

  void set_tag(int slot);
  void set_tag(llvm::StringRef name);
  void set_tag(llvm::StringRef name,
               llvm::StringRef prefix,
               bool may_need_quotes = true);

  void set_defn_range(const SourceRange& range);
  void set_llvm_range(const SourceRange& range);
  void set_source_range(const SourceRange& range);

  bool has_tag() const;
  llvm::StringRef get_tag() const;
  const SourceRange& get_defn_range() const;
  const SourceRange& get_llvm_range() const;
  const SourceRange& get_source_range() const;
};

} // namespace lb

#endif // LLVM_BROWSE_NAVIGABLE_H

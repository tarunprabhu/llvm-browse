#ifndef LLVM_BROWSE_NAVIGABLE_H
#define LLVM_BROWSE_NAVIGABLE_H

#include <string>
#include <vector>

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
class INavigable {
protected:
  // The tag is the "label" of the entity in the LLVM IR. For instructions, 
  // this might have the form "^%.+$" where the characters after the percent 
  // sign are typically numbers but they don't have to be. 
  // For struct types, this will be "%.+", for globals, this will be "^@.+$"
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
  SourceRange llvm_defn;

  // The LLVM range is the range in characters that the entity covers in
  // the IR. For functions, this is the entire body, for basic blocks, all
  // the characters from the start of the first instruction in the block to
  // the end of the last. It may be invalid for other entities
  SourceRange llvm_range;

  // The source defn is the range in characters in the source code that 
  // the definition of the entity covers. For functions and globals, this 
  // will simply span the beginning to the end of the name in the source code
  SourceRange source_defn;

  // The source range is the range in characters in the source code that the
  // entity covers. This is a somewhat more nebulous range because there may not
  // be a reasonable mapping from the source to LLVM. For instance, multiple
  // instructions could map to the same lines in the source code in which case
  // the source range of all those instructions are likely to be identical
  // Still, this is mainly here so we have a decent starting point at which
  // to position the cursor in the source even if we can't do anything else
  // beyond that 
  SourceRange source_range;

  // This is sort of messy because not everything that is navigable ought to
  // have a use. The exeception are struct types that also have a definition
  // but it doesn't make sense to have any uses for them. But it's a bit messy
  // to separate the two and still keep the type system straight (or - and this
  // is more likely - I am being particuarly dense)
  std::vector<SourceRange> m_uses;

public:
  using Iterator = decltype(m_uses)::const_iterator;

protected:
  INavigable() = default;

public:
  virtual ~INavigable() = default;

  // This gets used for both instructions and metadata nodes and for metadata
  // nodes, we have a different prefix
  void set_tag(unsigned slot, llvm::StringRef prefix = "%");
  void set_tag(llvm::StringRef name);
  void set_tag(llvm::StringRef name,
               llvm::StringRef prefix,
               bool may_need_quotes = true);
  
  void sort_uses();
  void add_use(const SourceRange& range);

  void set_llvm_defn(const SourceRange& range);
  void set_llvm_range(const SourceRange& range);
  void set_source_defn(const SourceRange& range);
  void set_source_range(const SourceRange& range);

  Iterator begin() const;
  Iterator end() const;
  llvm::iterator_range<Iterator> uses() const;
  unsigned get_num_uses() const;
  bool has_tag() const;
  llvm::StringRef get_tag() const;
  bool has_llvm_defn() const;
  bool has_llvm_range() const;
  bool has_source_defn() const;
  bool has_source_range() const;
  const SourceRange& get_llvm_defn() const;
  const SourceRange& get_llvm_range() const;
  const SourceRange& get_source_defn() const;
  const SourceRange& get_source_range() const;
};

} // namespace lb

#endif // LLVM_BROWSE_NAVIGABLE_H

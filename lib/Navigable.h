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
  SourceRange defn;
  SourceRange llvm;
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
  
  llvm::StringRef get_tag() const;
  const SourceRange& get_defn_range() const;
  const SourceRange& get_llvm_range() const;
  const SourceRange& get_source_range() const;
};

} // namespace lb

#endif // LLVM_BROWSE_NAVIGABLE_H

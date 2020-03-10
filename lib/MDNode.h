#ifndef LLVM_BROWSE_MDNODE_H
#define LLVM_BROWSE_MDNODE_H

#include <llvm/ADT/iterator_range.h>
#include <llvm/IR/Metadata.h>

#include "INavigable.h"
#include "IWrapper.h"
#include "SourceRange.h"
#include "Typedefs.h"

namespace lb {

class Module;

class alignas(ALIGN_OBJ) MDNode :
    public INavigable,
    public IWrapper<llvm::MDNode> {
protected:
  MDNode(const llvm::MDNode& llvm, unsigned slot, Module& module);

public:
  MDNode()          = delete;
  MDNode(MDNode&)   = delete;
  MDNode(MDNode&&)  = delete;
  virtual ~MDNode() = default;

  bool is_artificial() const;

public:
  static bool classof(const INavigable* v) {
    return v->get_kind() == EntityKind::MDNode;
  }

  static MDNode& make(const llvm::MDNode& llvm, unsigned slot, Module& module);
};

} // namespace lb

#endif // LLVM_BROWSE_MDNODE_H

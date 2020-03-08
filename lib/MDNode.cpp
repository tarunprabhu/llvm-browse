#include "MDNode.h"

namespace lb {

MDNode::MDNode(llvm::MDNode& llvm, unsigned slot, Module& module) :
    INavigable(EntityKind::MDNode), IWrapper<llvm::MDNode>(llvm, module) {
  set_tag(slot, "!");
}

bool
MDNode::is_artificial() const {
  // FIXME: At some point, it might make more sense to make a distinction
  // between "system" metadata and "user" metadata. But that probably doesn't
  // belong here anyway, and I doubt that something like that can ever happen
  // anyway
  return true;
}

} // namespace lb

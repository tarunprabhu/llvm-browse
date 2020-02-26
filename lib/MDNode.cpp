#include "MDNode.h"

namespace lb {

MDNode::MDNode(llvm::MDNode& llvm, unsigned slot, Module& module) :
    INavigable(), IWrapper<llvm::MDNode>(llvm, module) {
  set_tag(slot, "!");
}

} // namespace lb

#ifndef LLVM_BROWSE_MDNODE_H
#define LLVM_BROWSE_MDNODE_H

#include <llvm/ADT/iterator_range.h>
#include <llvm/IR/Metadata.h>

#include "INavigable.h"
#include "IWrapper.h"
#include "SourceRange.h"

namespace lb {

class Module;

class MDNode : public INavigable, public IWrapper<llvm::MDNode> {
public:
  MDNode(llvm::MDNode& llvm, unsigned slot, Module& module);
  virtual ~MDNode() = default;
};

} // namespace lb

#endif // LLVM_BROWSE_MDNODE_H

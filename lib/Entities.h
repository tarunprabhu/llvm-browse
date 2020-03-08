#ifndef LLVM_BROWSE_ENTITIES_H
#define LLVM_BROWSE_ENTITIES_H

namespace lb {

// Seems like I did weird things while trying to retain some of LLVM's
// class heirarchy when creating the wrappers.
// The Value class is intended to be a wrapper class for LLVM::Value. This
// works well except that llvm::Constant's are also llvm::Value's but in
// our case, they are not navigable. So we have a separate Navigable class
// However, we treat StructType's as being navigable because we can do a
// "goto-definition" on them, so we have overlaps without one being fully
// contained within another. This class keeps track of all the entities of
// any kind that are present within in this library for the purpose of
// casting

enum class EntityKind {
  GlobalAlias    = 0x1,
  Argument       = 0x2,
  BasicBlock     = 0x3,
  Comdat         = 0x4,
  Constant       = 0x5,
  Definition     = 0x6,
  Function       = 0x7,
  GlobalVariable = 0x8,
  Instruction    = 0x9,
  MDNode         = 0xA,
  Module         = 0xB,
  SourceRange    = 0xC,
  StructType     = 0xD,
  Use            = 0xE,
};

} // namespace lb

#endif // LLVM_BROWSE_ENTITIES_H
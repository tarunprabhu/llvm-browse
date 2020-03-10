#ifndef LLVM_BROWSE_USE_H
#define LLVM_BROWSE_USE_H

#include "LLVMRange.h"
#include "Typedefs.h"

#include <memory>

namespace lb {

class INavigable;
class Instruction;
class Module;

// The use is analogous to an LLVM use but we won't treat it as a wrapper
// around an LLVM use. Unlike an LLVM use, this has some "source information"
// associated with it, namely a range of offsets in the LLVM IR that it
// corresponds to.
//
class alignas(ALIGN_OBJ) Use {
protected:
  // The range in the LLVM IR that this use corresponds to
  lb::LLVMRange range;

  // The value at this location in the IR
  const INavigable& used;

  // The instruction is for when this Use corresponds to an instruction
  // operand. A use could also be present in two other places:
  //  - In an llvm::ConstantExpr but these don't have a "definition", so aren't
  //		navigable in which case there isn't much we can do about it
  //  - In an llvm::MDNode. In this case, the MDNode could have uses in a number
  //    of different places including other MDNodes but also in metadata
  //    attached to instructions. We don't care about sorting those out,
  ///   so we don't bother with them
  const Instruction* inst;

protected:
  Use(Offset begin,
      Offset end,
      const INavigable& used,
      const Instruction* inst = nullptr);

public:
  Use()          = delete;
  Use(Use&)      = delete;
  Use(Use&&)     = delete;
  virtual ~Use() = default;

  bool has_user() const;
  Offset get_begin() const;
  Offset get_end() const;
  const INavigable& get_used() const;
  const Instruction* get_instruction() const;

  operator bool() const {
    return (range.get_begin() > 0) and (range.get_end() > 0);
  }

public:
  static Use& make(Offset begin,
                   Offset end,
                   const INavigable& used,
                   Module& module,
                   const Instruction* inst = nullptr);
};

} // namespace lb

#endif // LLVM_BROWSE_USE_H
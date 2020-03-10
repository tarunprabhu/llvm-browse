#ifndef LLVM_BROWSE_COMDAT_H
#define LLVM_BROWSE_COMDAT_H

#include "INavigable.h"
#include "IWrapper.h"
#include "Typedefs.h"

#include <llvm/IR/Comdat.h>
#include <llvm/IR/GlobalObject.h>

namespace lb {

class Module;
class Value;

// Comdats are weird because they are not really values but we want to be able
// to do a "goto definition" on them in which case they will go to the
// corresponding function/global that the Comdat represents. But they never
// get 'used' anywhere and aren't actually uses
class alignas(ALIGN_OBJ) Comdat :
    public INavigable,
    public IWrapper<llvm::Comdat> {
protected:
  const llvm::GlobalObject& target;
  LLVMRange self_defn;

protected:
  Comdat(const llvm::Comdat& comdat,
         const llvm::GlobalObject& target,
         Module& module);

public:
  Comdat()          = delete;
  Comdat(Comdat&)   = delete;
  Comdat(Comdat&&)  = delete;
  virtual ~Comdat() = default;

  // Because we treat this as a "special use", the definition will actually be
  // the definition of the target and this function will be set the
  // definition of the Comdat itself
  void set_self_llvm_defn(const LLVMRange& defn);

  const LLVMRange& get_self_llvm_defn() const;

  // These will not return any Value. They must be either Function or
  // GlobalVariable but we haven't recreated all of LLVM's heirarchy so
  // we will have to do with value
  const Value& get_target() const;
  template<typename T>
  const T& get_target_as() const;

public:
  static bool classof(const INavigable* v) {
    return v->get_kind() == EntityKind::Comdat;
  }

  static Comdat& make(const llvm::Comdat& comdat,
                      const llvm::GlobalObject& target,
                      Module& module);
};

} // namespace lb

#endif // LLVM_BROWSE_COMDAT_H

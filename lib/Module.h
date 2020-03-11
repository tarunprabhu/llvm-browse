#ifndef LLVM_BROWSE_MODULE_H
#define LLVM_BROWSE_MODULE_H

#include <llvm/ADT/iterator_range.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/MemoryBuffer.h>

#include <map>
#include <memory>
#include <set>
#include <vector>

#include "Argument.h"
#include "BasicBlock.h"
#include "Comdat.h"
#include "Definition.h"
#include "Errors.h"
#include "Function.h"
#include "GlobalAlias.h"
#include "GlobalVariable.h"
#include "Instruction.h"
#include "Iterator.h"
#include "LLVMRange.h"
#include "MDNode.h"
#include "Parser.h"
#include "StructType.h"
#include "Typedefs.h"
#include "Use.h"

namespace lb {

// Wrapper class around an LLVM module. The wrappers around the LLVM classes
// contains "LLVM source code" information, mainly just line and column
// numbers. Keeping it in here instead of separately so that if we need
// additional flags for the entities, they can be kept in one place that
// makes sense
//
class alignas(ALIGN_OBJ) Module {
protected:
  // Managed memory for objects that will always live for the duration of
  // the object but will never be touched directly
  std::unique_ptr<llvm::LLVMContext> context;
  std::unique_ptr<llvm::Module> llvm;
  std::unique_ptr<llvm::MemoryBuffer> buffer;

  // These are all the objects that the module owns. Not all are directly
  // exposed from here Everything in these arrays
  // needs to be freed in the destructor. At some point, I'll create an
  // iterator that exposes a reference to these instead of the pointer
  std::vector<std::unique_ptr<GlobalAlias>> m_aliases;
  std::vector<std::unique_ptr<Comdat>> m_comdats;
  // We make a distinction between functions and declarations mainly to search
  // Functions with definitions have a span and need to be ordered
  // Declarations have no body and the best we can do is find uses of it
  std::vector<std::unique_ptr<Function>> m_decls;
  std::vector<std::unique_ptr<Function>> m_functions;
  std::vector<std::unique_ptr<GlobalVariable>> m_globals;
  std::vector<std::unique_ptr<MDNode>> m_metadata;
  std::vector<std::unique_ptr<StructType>> m_structs;

  // The uses are guaranteed not to overlap and are sorted in the order in
  // which they appear in the IR
  std::vector<std::unique_ptr<Use>> uses;

  // These are the definitions of the Navigable entities in the IR.
  // These are guaranteed not to overlap and are sorted in the order in
  // which they appear in the IR
  std::vector<std::unique_ptr<Definition>> defs;

  // Wrapper lookup maps
  std::map<const llvm::Comdat*, Comdat*> cmap;
  std::map<const llvm::MDNode*, MDNode*> mmap;
  std::map<llvm::StructType*, StructType*> tmap;
  std::map<const llvm::Value*, Value*> vmap;

  // The file names seen in the source
  // We want to keep this because the debug information is not consistent 
  // in the use of getFilename() and getDirectory(). In some cases, 
  // getFilename() returns just the filename but in others, it returns the 
  // full path to the file. So we keep strings of all the filenames here 
  // so we can return llvm::StringRef where needed instead of copying the 
  // same string for every SourceRange object
  std::set<std::string> files;

public:
  using AliasIterator    = DerefIterator<decltype(m_aliases)::const_iterator>;
  using ComdatIterator   = DerefIterator<decltype(m_comdats)::const_iterator>;
  using DeclIterator     = DerefIterator<decltype(m_decls)::const_iterator>;
  using FunctionIterator = DerefIterator<decltype(m_functions)::const_iterator>;
  using GlobalIterator   = DerefIterator<decltype(m_globals)::const_iterator>;
  using MetadataIterator = DerefIterator<decltype(m_metadata)::const_iterator>;
  using StructIterator   = DerefIterator<decltype(m_structs)::const_iterator>;
  using UseIterator      = DerefIterator<decltype(uses)::iterator>;
  using UseConstIterator = DerefIterator<decltype(uses)::const_iterator>;
  using DefIterator      = DerefIterator<decltype(defs)::iterator>;
  using DefConstIterator = DerefIterator<decltype(defs)::const_iterator>;

protected:
  Module(std::unique_ptr<llvm::Module> module,
         std::unique_ptr<llvm::LLVMContext> context,
         std::unique_ptr<llvm::MemoryBuffer> mbuf);

  template<typename T = Value>
  T& get(const llvm::Value& llvm) {
    return *llvm::cast<T>(vmap.at(&llvm));
  }

  template<typename T = Value>
  T& get(const llvm::Value* llvm) {
    return llvm::cast<T>(vmap.at(llvm));
  }

  template<typename T = Value>
  const T& get(const llvm::Value& llvm) const {
    return *llvm::cast<T>(vmap.at(&llvm));
  }

  template<typename T = Value>
  const T* get(const llvm::Value* llvm) const {
    return llvm::cast<T>(vmap.at(llvm));
  }

  void sort();

  bool check_range(Offset begin, Offset end, llvm::StringRef tag) const;
  bool check_uses(const INavigable& navigable) const;
  bool check_navigable(const INavigable& navigable) const;

  Argument& get(const llvm::Argument& llvm);
  BasicBlock& get(const llvm::BasicBlock& llvm);
  Comdat& get(const llvm::Comdat& llvm);
  Instruction& get(const llvm::Instruction& llvm);
  Function& get(const llvm::Function& llvm);
  GlobalAlias& get(const llvm::GlobalAlias& llvm);
  GlobalVariable& get(const llvm::GlobalVariable& llvm);
  MDNode& get(const llvm::MDNode& llvm);
  StructType& get(llvm::StructType* llvm);
  Value& get(const llvm::Value& llvm);

public:
  Module()               = delete;
  Module(const Module&)  = delete;
  Module(const Module&&) = delete;
  virtual ~Module()      = default;

  llvm::StringRef get_full_path(llvm::StringRef dir, llvm::StringRef file);
  bool contains(const llvm::Value& llvm) const;
  bool contains(const llvm::MDNode& llvm) const;

  llvm::MemoryBufferRef get_code_buffer() const;
  llvm::StringRef get_code() const;

  const Argument& get(const llvm::Argument& llvm) const;
  const BasicBlock& get(const llvm::BasicBlock& llvm) const;
  const Comdat& get(const llvm::Comdat& llvm) const;
  const Instruction& get(const llvm::Instruction& llvm) const;
  const Function& get(const llvm::Function& llvm) const;
  const GlobalAlias& get(const llvm::GlobalAlias& llvm) const;
  const GlobalVariable& get(const llvm::GlobalVariable& llvm) const;
  const MDNode& get(const llvm::MDNode& llvm) const;
  const Value& get(const llvm::Value& llvm) const;
  const StructType& get(llvm::StructType* llvm) const;

  llvm::iterator_range<AliasIterator> aliases() const;
  llvm::iterator_range<ComdatIterator> comdats() const;
  llvm::iterator_range<DeclIterator> decls() const;
  llvm::iterator_range<FunctionIterator> functions() const;
  llvm::iterator_range<GlobalIterator> globals() const;
  llvm::iterator_range<MetadataIterator> metadata() const;
  llvm::iterator_range<StructIterator> structs() const;

  unsigned get_num_aliases() const;
  unsigned get_num_comdats() const;
  unsigned get_num_functions() const;
  unsigned get_num_globals() const;
  unsigned get_num_metadata() const;
  unsigned get_num_structs() const;

  const Use* get_use_at(Offset offset) const;
  const Definition* get_definition_at(Offset offset) const;
  const Instruction* get_instruction_at(Offset offset) const;
  const BasicBlock* get_block_at(Offset offset) const;
  const Function* get_function_at(Offset offset) const;
  const Comdat* get_comdat_at(Offset offset) const;

  llvm::Module& get_llvm();
  const llvm::Module& get_llvm() const;

  bool check_top_level() const;
  bool check_all(bool metadata) const;

  explicit operator bool() const {
    return llvm.get();
  }

public:
  static std::unique_ptr<const Module> create(const std::string& file);

public:
  friend class Parser;
  friend Argument&
  Argument::make(const llvm::Argument& llvm_a, Function& f, Module& module);
  friend BasicBlock& BasicBlock::make(const llvm::BasicBlock& llvm_bb,
                                      Function& f,
                                      Module& module);
  friend Comdat& Comdat::make(const llvm::Comdat& llvm_c,
                              const llvm::GlobalObject& target,
                              Module& module);
  friend Function& Function::make(const llvm::Function& llvm_f, Module& module);
  friend GlobalAlias& GlobalAlias::make(const llvm::GlobalAlias& llvm_a,
                                        Module& module);
  friend GlobalVariable&
  GlobalVariable::make(const llvm::GlobalVariable& llvm_g, Module& module);
  friend Instruction& Instruction::make(const llvm::Instruction& llvm_i,
                                        BasicBlock& bb,
                                        Function& f,
                                        Module& module);
  friend MDNode&
  MDNode::make(const llvm::MDNode& llvm_md, unsigned slot, Module& module);
  friend StructType& StructType::make(llvm::StructType* llvm_sty,
                                      Module& module);

  friend Use& Use::make(Offset begin,
                        Offset end,
                        const INavigable& value,
                        Module& module,
                        const Instruction* inst);
  friend Definition& Definition::make(Offset begin,
                                      Offset end,
                                      const INavigable& defined,
                                      Module& module);
};

} // namespace lb

#endif // LLVM_BROWSE_MODULE_H

#include "Module.h"
#include "Argument.h"
#include "BasicBlock.h"
#include "Function.h"
#include "GlobalAlias.h"
#include "GlobalVariable.h"
#include "Instruction.h"
#include "MDNode.h"
#include "Parser.h"
#include "StructType.h"

#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>

#include <glibmm.h>

namespace lb {

Module::Module(std::unique_ptr<llvm::Module> module,
               llvm::LLVMContext& context) :
    context(context),
    llvm(std::move(module)) {
  ;
}

BufferId
Module::get_next_available_id() {
  return buffers.size();
}

bool
Module::contains(const llvm::Value& llvm) const {
  return vmap.find(&llvm) != vmap.end();
}

bool
Module::contains(const llvm::MDNode& llvm) const {
  return mmap.find(&llvm) != mmap.end();
}
  
bool
Module::contains_main() const {
  return buffers.find(Module::get_main_id()) != buffers.end();
}

Argument&
Module::add(llvm::Argument& llvm) {
  return add<Argument>(llvm);
}

BasicBlock&
Module::add(llvm::BasicBlock& llvm) {
  return add<BasicBlock>(llvm);
}

Function&
Module::add(llvm::Function& llvm) {
  Function& f = add<Function>(llvm);
  m_functions.push_back(&f);
  return f;
}

GlobalAlias&
Module::add(llvm::GlobalAlias& llvm) {
  GlobalAlias& a = add<GlobalAlias>(llvm);
  m_aliases.push_back(&a);
  return a;
}

GlobalVariable&
Module::add(llvm::GlobalVariable& llvm) {
  GlobalVariable& g = add<GlobalVariable>(llvm);
  m_globals.push_back(&g);
  return g;
}

Instruction&
Module::add(llvm::Instruction& llvm) {
  return add<Instruction>(llvm);
}

MDNode&
Module::add(llvm::MDNode& llvm, unsigned slot) {
  auto* ptr = new MDNode(llvm, slot, *this);
  mds.emplace_back(ptr);
  m_metadata.push_back(ptr);
  mmap[&llvm] = ptr;
  return *ptr;
}

StructType&
Module::add(llvm::StructType* llvm) {
  auto* ptr = new StructType(llvm, *this);
  struct_types.emplace_back(ptr);
  m_structs.push_back(ptr);
  tmap[llvm] = ptr;
  return *ptr;
}

BufferId
Module::add_file(const std::string& file) {
  BufferId id = Module::get_invalid_id();
  if(llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> fileOrErr
     = llvm::MemoryBuffer::getFile(file)) {
    id = get_next_available_id();
    buffers.emplace(std::make_pair(id, std::move(fileOrErr.get())));
  }
  return id;
}

BufferId
Module::add_main(const std::string& file) {
  BufferId id = Module::get_main_id();
  // FIXME: Add error checking and reporting. Should only set the main buffer
  // exactly once
  if(llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> fileOrErr
     = llvm::MemoryBuffer::getFile(file)) {
    buffers.emplace(std::make_pair(id, std::move(fileOrErr.get())));
  }
  return id;
}

BufferId
Module::add_main(std::unique_ptr<llvm::MemoryBuffer> mbuf) {
  BufferId id = Module::get_main_id();
  // FIXME: Add error checking and reporting. Should set the main buffer
  // exactly once
  buffers.emplace(std::make_pair(id, std::move(mbuf)));

  return id;
}

StructType&
Module::get(llvm::StructType* llvm) {
  return *tmap.at(llvm);
}

Argument&
Module::get(const llvm::Argument& llvm) {
  return get<Argument>(llvm);
}

BasicBlock&
Module::get(const llvm::BasicBlock& llvm) {
  return get<BasicBlock>(llvm);
}

Instruction&
Module::get(const llvm::Instruction& llvm) {
  return get<Instruction>(llvm);
}

Function&
Module::get(const llvm::Function& llvm) {
  return get<Function>(llvm);
}

GlobalAlias&
Module::get(const llvm::GlobalAlias& llvm) {
  return get<GlobalAlias>(llvm);
}

GlobalVariable&
Module::get(const llvm::GlobalVariable& llvm) {
  return get<GlobalVariable>(llvm);
}

MDNode&
Module::get(const llvm::MDNode& llvm) {
  return *mmap.at(&llvm);
}

Value&
Module::get(const llvm::Value& llvm) {
  return get<Value>(llvm);
}

const StructType&
Module::get(llvm::StructType* llvm) const {
  return *tmap.at(llvm);
}

const Argument&
Module::get(const llvm::Argument& llvm) const {
  return get<Argument>(llvm);
}

const BasicBlock&
Module::get(const llvm::BasicBlock& llvm) const {
  return get<BasicBlock>(llvm);
}

const Instruction&
Module::get(const llvm::Instruction& llvm) const {
  return get<Instruction>(llvm);
}

const Function&
Module::get(const llvm::Function& llvm) const {
  return get<Function>(llvm);
}

const GlobalAlias&
Module::get(const llvm::GlobalAlias& llvm) const {
  return get<GlobalAlias>(llvm);
}

const GlobalVariable&
Module::get(const llvm::GlobalVariable& llvm) const {
  return get<GlobalVariable>(llvm);
}

const MDNode&
Module::get(const llvm::MDNode& llvm) const {
  return *mmap.at(&llvm);
}

const Value&
Module::get(const llvm::Value& llvm) const {
  return get<Value>(llvm);
}

llvm::MemoryBufferRef
Module::get_buffer(BufferId id) const {
  return buffers.at(id)->getMemBufferRef();
}

llvm::StringRef
Module::get_contents(BufferId id) const {
  return buffers.at(id)->getBuffer();
}

llvm::iterator_range<Module::AliasIterator>
Module::aliases() const {
  return llvm::iterator_range<AliasIterator>(m_aliases);
}

llvm::iterator_range<Module::FunctionIterator>
Module::functions() const {
  return llvm::iterator_range<FunctionIterator>(m_functions);
}

llvm::iterator_range<Module::GlobalIterator>
Module::globals() const {
  return llvm::iterator_range<GlobalIterator>(m_globals);
}

llvm::iterator_range<Module::MetadataIterator>
Module::metadata() const {
  return llvm::iterator_range<MetadataIterator>(m_metadata);
}

llvm::iterator_range<Module::StructIterator>
Module::structs() const {
  return llvm::iterator_range<StructIterator>(m_structs);
}

llvm::Module&
Module::get_llvm() {
  return *llvm;
}

const llvm::Module&
Module::get_llvm() const {
  return *llvm;
}

bool
Module::check_range(const SourceRange& range, llvm::StringRef tag) const {
  size_t begin          = range.get_begin();
  size_t end            = range.get_end();
  llvm::StringRef slice = get_contents().substr(begin, end - begin);
  if(slice != tag)
    return false;
  return true;
}

bool
Module::check_navigable(const INavigable& n) const {
  llvm::StringRef tag = n.get_tag();
  if(const SourceRange& range = n.get_defn_range()) {
    if(not check_range(range, tag)) {
      size_t begin = range.get_begin();
      size_t end   = range.get_end();
      std::string buf;
      llvm::raw_string_ostream ss(buf);
      ss << "Definition mismatch\n"
         << "  Range:    " << begin << ", " << end << "\n"
         << "  Expected: " << tag << "\n"
         << "  Got:      " << get_contents().substr(begin, end - begin);
      ss.flush();
      g_critical("\n%s", buf.c_str());
      return false;
    }
  } else {
    g_critical("Invalid definition: %s", tag.data());
    return false;
  }
  return true;
}

bool
Module::check_uses(const INavigable& n) const {
  for(const SourceRange& use : n.uses()) {
    if(not check_range(use, n.get_tag())) {
      size_t begin = use.get_begin();
      size_t end   = use.get_end();
      std::string buf;
      llvm::raw_string_ostream ss(buf);
      ss << "Use mismatch\n"
         << "  Range:    " << begin << ", " << end << "\n"
         << "  Expected: " << n.get_tag() << "\n"
         << "  Got:      " << get_contents().substr(begin, end - begin);
      ss.flush();
      g_critical("\n%s", buf.c_str());
      return false;
    }
  }

  // TODO:
  // Ideally, we should also try to check that the number of uses matches
  // with LLVM. But that might result in many mismatches because there may
  // be uses in the debug metadata that will be skipped because the
  // debug metadata is not currently browsable

  return true;
}

bool
Module::check_top_level() const {
  g_message("Checking top-level entities");
  for(const StructType* sty : structs())
    if(not check_navigable(*sty))
      return false;
  for(const GlobalAlias* a : aliases())
    if(not check_navigable(*a))
      return false;
  for(const GlobalVariable* g : globals())
    if(not check_navigable(*g))
      return false;
  for(const Function* f : functions())
    if(not check_navigable(*f))
      return false;
  return true;
}

bool
Module::check_all(bool check_metadata) const {
  if(not check_top_level())
    return false;
  g_message("Checking uses of aliases");
  for(const GlobalAlias* a : aliases())
    if(not check_uses(*a))
      return false;
  g_message("Checking uses of globals");
  for(const GlobalVariable* g : globals())
    if(not check_uses(*g))
      return false;
  g_message("Checking uses of functions");
  for(const Function* f : functions()) {
    if(not check_uses(*f))
      return false;
    for(const Argument* arg : f->arguments())
      if(not check_uses(*arg))
        return false;
    for(const BasicBlock* bb : f->blocks()) {
      // We can't check the names of a basic block because the definition
      // has no place where it is consistently declared. We probably need
      // a better test case for this
      if(not check_uses(*bb))
        return false;
      for(const Instruction* inst : bb->instructions()) {
        if((not inst->get_llvm().getType()->isVoidTy())
           and (not check_navigable(*inst)))
          return false;
        if(not check_uses(*inst))
          return false;
      }
    }
  }
  if(check_metadata) {
    g_message("Checking metadata");
    for(const MDNode* md : metadata())
      if(not check_navigable(*md) or not check_uses(*md)) 
        return false;
  }
  return true;
}

std::unique_ptr<Module>
Module::create(const std::string& file, llvm::LLVMContext& context) {
  std::unique_ptr<Module> module(nullptr);

  g_message("Opening file");
  if(llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> file_or_err
     = llvm::MemoryBuffer::getFile(file)) {
    std::unique_ptr<llvm::MemoryBuffer> fbuf = std::move(file_or_err.get());
    std::unique_ptr<llvm::MemoryBuffer> mbuf(nullptr);
    std::unique_ptr<llvm::Module> llvm(nullptr);
    Parser parser;

    // We need this check because llvm::isBitcode() assumes that the buffer is
    // at least 4 bytes
    if(fbuf->getBufferSize() > 4) {
      if(llvm::isBitcode(
             reinterpret_cast<const unsigned char*>(fbuf->getBufferStart()),
             reinterpret_cast<const unsigned char*>(fbuf->getBufferEnd())))
        std::tie(llvm, mbuf) = parser.parse_bc(std::move(fbuf), context);
      else
        std::tie(llvm, mbuf) = parser.parse_ir(std::move(fbuf), context);

      if(llvm) {
        module.reset(new Module(std::move(llvm), context));
        module->add_main(std::move(mbuf));
        parser.link(*module);
      }
    } else {
      g_error("Could not find LLVM bitcode or IR");
    }
  } else {
    g_error("Could not open file");
  }

  return module;
}

} // namespace lb

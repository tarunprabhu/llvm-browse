#include "Module.h"
#include "Argument.h"
#include "BasicBlock.h"
#include "Function.h"
#include "GlobalAlias.h"
#include "GlobalVariable.h"
#include "Instruction.h"
#include "Logging.h"
#include "MDNode.h"
#include "Parser.h"
#include "StructType.h"

#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>

using llvm::cast;
using llvm::dyn_cast;
using llvm::isa;

namespace lb {

Module::Module(std::unique_ptr<llvm::Module> module,
               std::unique_ptr<llvm::LLVMContext> context,
               std::unique_ptr<llvm::MemoryBuffer> mbuf) :
    context(std::move(context)),
    llvm(std::move(module)), buffer(std::move(mbuf)) {
  ;
}

bool
Module::contains(const llvm::Value& llvm) const {
  return vmap.find(&llvm) != vmap.end();
}

bool
Module::contains(const llvm::MDNode& llvm) const {
  return mmap.find(&llvm) != mmap.end();
}

Argument&
Module::add(llvm::Argument& llvm, Function& f) {
  return add<Argument>(llvm, f);
}

BasicBlock&
Module::add(llvm::BasicBlock& llvm, Function& f) {
  return add<BasicBlock>(llvm, f);
}

Comdat&
Module::add(llvm::Comdat& llvm, llvm::GlobalObject& g) {
  auto* ptr = new Comdat(llvm, g, *this);
  comdat_ptrs.emplace_back(ptr);
  m_comdats.push_back(ptr);
  cmap[&llvm] = ptr;
  return *ptr;
}

Function&
Module::add(llvm::Function& llvm) {
  Function& f = add<Function>(llvm);
  m_functions.push_back(&f);
  func_spans.push_back(&f);
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
Module::add(llvm::Instruction& llvm, Function& f) {
  return add<Instruction>(llvm, f);
}

MDNode&
Module::add(llvm::MDNode& llvm, unsigned slot) {
  auto* ptr = new MDNode(llvm, slot, *this);
  mdnode_ptrs.emplace_back(ptr);
  m_metadata.push_back(ptr);
  mmap[&llvm] = ptr;
  return *ptr;
}

StructType&
Module::add(llvm::StructType* llvm) {
  auto* ptr = new StructType(llvm, *this);
  struct_ptrs.emplace_back(ptr);
  m_structs.push_back(ptr);
  tmap[llvm] = ptr;
  return *ptr;
}

Use&
Module::add_use(uint64_t begin,
                uint64_t end,
                const INavigable& value,
                const Instruction* inst) {
  Use* use = new Use(begin, end, value, inst);
  use_ptrs.emplace_back(use);
  uses.push_back(use);
  return *use;
}

Definition&
Module::add_definition(uint64_t begin,
                       uint64_t end,
                       const INavigable& defined) {
  Definition* defn = new Definition(begin, end, defined);
  def_ptrs.emplace_back(defn);
  defns.push_back(defn);
  return *defn;
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

Comdat&
Module::get(const llvm::Comdat& llvm) {
  return *cmap.at(&llvm);
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

const Comdat&
Module::get(const llvm::Comdat& llvm) const {
  return *cmap.at(&llvm);
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
Module::get_code_buffer() const {
  return buffer->getMemBufferRef();
}

llvm::StringRef
Module::get_code() const {
  return buffer->getBuffer();
}

llvm::iterator_range<Module::AliasIterator>
Module::aliases() const {
  return llvm::iterator_range<AliasIterator>(m_aliases);
}

llvm::iterator_range<Module::ComdatIterator>
Module::comdats() const {
  return llvm::iterator_range<ComdatIterator>(m_comdats);
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

unsigned
Module::get_num_aliases() const {
  return m_aliases.size();
}

unsigned
Module::get_num_comdats() const {
  return m_comdats.size();
}

unsigned
Module::get_num_functions() const {
  return m_functions.size();
}

unsigned
Module::get_num_globals() const {
  return m_globals.size();
}

unsigned
Module::get_num_metadata() const {
  return m_metadata.size();
}

unsigned
Module::get_num_structs() const {
  return m_structs.size();
}

template<typename T,
         std::enable_if_t<!std::is_base_of<INavigable, T>::value, int> = 0>
static uint64_t
get_offset_begin(const T* ptr) {
  return ptr->get_begin();
}

static uint64_t
get_offset_begin(const INavigable* v) {
  return v->get_llvm_span().get_begin();
}

template<typename T,
         std::enable_if_t<!std::is_base_of<INavigable, T>::value, int> = 0>
static uint64_t
get_offset_end(const T* ptr) {
  return ptr->get_end();
}

static uint64_t
get_offset_end(const INavigable* v) {
  return v->get_llvm_span().get_end();
}

template<typename T>
static const T*
bin_search(uint64_t offset,
           const std::vector<const T*>& vec,
           unsigned left,
           unsigned right) {
  if(left > right)
    return nullptr;

  unsigned mid   = std::floor(left + right) / 2;
  uint64_t begin = get_offset_begin(vec[mid]);
  uint64_t end   = get_offset_end(vec[mid]);

  if(offset < begin)
    return bin_search(offset, vec, 0, mid - 1);
  else if(offset > end)
    return bin_search(offset, vec, mid + 1, right);
  else
    return vec[mid];
}

template<typename T>
static const T*
bin_search(uint64_t offset, const std::vector<const T*>& vec) {
  return bin_search(offset, vec, 0, vec.size() - 1);
}

const Use*
Module::get_use_at(uint64_t offset) const {
  if(not offset)
    return nullptr;

  return bin_search(offset, uses);
}

const Definition*
Module::get_definition_at(uint64_t offset) const {
  if(not offset)
    return nullptr;

  return bin_search(offset, defns);
}

const Instruction*
Module::get_instruction_at(uint64_t offset) const {
  if(not offset)
    return nullptr;

  // Just do a linear search for the instructions because the functions
  // won't be large enough to warrant the extra memory cost of keeping all
  // the instructions in sorted order
  if(const Function* f = get_function_at(offset))
    for(const BasicBlock* bb : f->blocks())
      for(const Instruction* inst : bb->instructions())
        if(const LLVMRange& span = inst->get_llvm_span())
          if((offset >= span.get_begin()) and (offset <= span.get_end()))
            return inst;

  return nullptr;
}

const BasicBlock*
Module::get_block_at(uint64_t offset) const {
  if(not offset)
    return nullptr;

  // Just do a linear search for the basic blocks because the functions
  // won't be large enough to warrant the extra memory cost of keeping all
  // the basic blocks in sorted order
  if(const Function* f = get_function_at(offset))
    for(const BasicBlock* bb : f->blocks())
      if(const LLVMRange& span = bb->get_llvm_span())
        if((offset >= span.get_begin()) and (offset <= span.get_end()))
          return bb;
  return nullptr;
}

const Function*
Module::get_function_at(uint64_t offset) const {
  if(not offset)
    return nullptr;

  return bin_search(offset, func_spans);
}

void
Module::sort_uses() {
  message() << "Sorting all uses\n";

  std::sort(uses.begin(),
            uses.end(),
            [](const Use* l, const Use* r) {
              return l->get_begin() < r->get_begin();
            });

  message() << "Sort entity uses\n";
  for(auto& i : vmap) {
    Value* v = i.second;
    if(auto* f = dyn_cast<Function>(v))
      f->sort_uses();
    else if(auto* arg = dyn_cast<Argument>(v))
      arg->sort_uses();
    else if(auto* bb = dyn_cast<BasicBlock>(v))
      bb->sort_uses();
    else if(auto* inst = dyn_cast<Instruction>(v))
      inst->sort_uses();
    else if(auto* alias = dyn_cast<GlobalAlias>(v))
      alias->sort_uses();
    else if(auto* g = dyn_cast<GlobalVariable>(v))
      g->sort_uses();
  }
  for(auto& i : tmap)
    i.second->sort_uses();
  for(auto& i : mmap)
    i.second->sort_uses();

  // First
}

void
Module::sort_definitions() {
  message() << "Sorting definitions\n";

  std::sort(defns.begin(),
            defns.end(),
            [](const Definition* l,
               const Definition* r) {
              return l->get_begin() < r->get_begin();
            });
}

void
Module::sort_func_spans() {
  message() << "Sorting function spans\n";

  // Remove any functions that don't have a span.
  std::remove_if(func_spans.begin(),
                 func_spans.end(),
                 [](const Function* f) { return not f->get_llvm_span(); });

  std::sort(func_spans.begin(),
            func_spans.end(),
            [](const Function* l, const Function* r) {
              return l->get_llvm_span().get_begin()
                     < r->get_llvm_span().get_begin();
            });
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
Module::check_range(uint64_t begin, uint64_t end, llvm::StringRef tag) const {
  return get_code().substr(begin, end - begin) == tag;
}

bool
Module::check_navigable(const INavigable& n) const {
  llvm::StringRef tag = n.get_tag();
  if(const Definition& defn = n.get_llvm_defn()) {
    uint64_t begin = defn.get_begin();
    uint64_t end   = defn.get_end();
    if(not check_range(begin, end, tag)) {
      critical() << "Definition mismatch" << endl
                 << "  Range:    " << begin << ", " << end << endl
                 << "  Expected: " << tag << endl
                 << "  Got:      " << get_code().substr(begin, end - begin)
                 << "\n";
      return false;
    }
  } else {
    critical() << "Invalid definition: " << tag << "\n";
    return false;
  }
  return true;
}

bool
Module::check_uses(const INavigable& n) const {
  for(const Use* use : n.uses()) {
    uint64_t begin = use->get_begin();
    uint64_t end   = use->get_end();
    if(not check_range(begin, end, n.get_tag())) {
      critical() << "Use mismatch" << endl
                 << "  Range:    " << begin << ", " << end << endl
                 << "  Expected: " << n.get_tag() << endl
                 << "  Got:      " << get_code().substr(begin, end - begin)
                 << "\n";
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
  message() << "Checking top-level entities\n";
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
  message() << "Checking uses of aliases\n";
  for(const GlobalAlias* a : aliases())
    if(not check_uses(*a))
      return false;
  message() << "Checking uses of globals\n";
  for(const GlobalVariable* g : globals())
    if(not check_uses(*g))
      return false;
  message() << "Checking uses of functions\n";
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
    message() << "Checking metadata\n";
    for(const MDNode* md : metadata())
      if(not check_navigable(*md) or not check_uses(*md))
        return false;
  }
  return true;
}

std::unique_ptr<const Module>
Module::create(const std::string& file) {
  std::unique_ptr<Module> module(nullptr);
  std::unique_ptr<llvm::LLVMContext> context(new llvm::LLVMContext());

  message() << "Opening file\n";
  if(llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> file_or_err
     = llvm::MemoryBuffer::getFile(file)) {
    std::unique_ptr<llvm::MemoryBuffer> fbuf = std::move(file_or_err.get());
    std::unique_ptr<llvm::MemoryBuffer> mbuf(nullptr);
    std::unique_ptr<llvm::Module> llvm(nullptr);

    // We need this check because llvm::isBitcode() assumes that the buffer is
    // at least 4 bytes
    if(fbuf->getBufferSize() > 4) {
      Parser parser;
      if(llvm::isBitcode(
             reinterpret_cast<const unsigned char*>(fbuf->getBufferStart()),
             reinterpret_cast<const unsigned char*>(fbuf->getBufferEnd())))
        std::tie(llvm, mbuf) = parser.parse_bc(std::move(fbuf), *context);
      else
        std::tie(llvm, mbuf) = parser.parse_ir(std::move(fbuf), *context);

      if(llvm) {
        module.reset(
            new Module(std::move(llvm), std::move(context), std::move(mbuf)));
        parser.link(*module);

        module->sort_uses();
        module->sort_definitions();
        module->sort_func_spans();
      }
    } else {
      critical() << "Could not find LLVM bitcode or IR\n";
    }
  } else {
    error() << "Could not open file: " << file << "\n";
  }

  return module;
}

} // namespace lb

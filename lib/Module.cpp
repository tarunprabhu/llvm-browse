#include "Module.h"
#include "Function.h"
#include "GlobalVariable.h"
#include "Parser.h"
#include "StructType.h"

#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>

#include <glib.h>

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
Module::contains_main() const {
  return buffers.find(Module::get_main_id()) != buffers.end();
}

StructType&
Module::add(llvm::StructType* llvm) {
  auto* ptr = new StructType(llvm, *this);
  struct_types.emplace_back(ptr);
  tmap[llvm] = ptr;
  return *ptr;
}

Function&
Module::add(const llvm::Function& llvm) {
  Function& f = add<Function>(llvm);
  fptrs.push_back(&f);
  return f;
}

GlobalVariable&
Module::add(const llvm::GlobalVariable& llvm) {
  GlobalVariable& g = add<GlobalVariable>(llvm);
  gptrs.push_back(&g);
  return g;
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

const StructType&
Module::get(llvm::StructType* llvm) const {
  return *tmap.at(llvm);
}

llvm::MemoryBufferRef
Module::get_buffer(BufferId id) const {
  return buffers.at(id)->getMemBufferRef();
}

llvm::StringRef
Module::get_contents(BufferId id) const {
  return buffers.at(id)->getBuffer();
}

llvm::iterator_range<Module::FunctionIterator>
Module::functions() const {
  return llvm::iterator_range<Module::FunctionIterator>(fptrs);
}

llvm::iterator_range<Module::GlobalIterator>
Module::globals() const {
  return llvm::iterator_range<Module::GlobalIterator>(gptrs);
}

llvm::iterator_range<Module::StructTypeIterator>
Module::structs() const {
  return llvm::iterator_range<Module::StructTypeIterator>(sptrs);
}

llvm::Module&
Module::get_llvm() {
  return *llvm;
}

const llvm::Module&
Module::get_llvm() const {
  return *llvm;
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

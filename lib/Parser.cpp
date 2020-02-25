#include "Parser.h"
#include "Argument.h"
#include "BasicBlock.h"
#include "Function.h"
#include "GlobalVariable.h"
#include "Instruction.h"

#include <glib.h>

#include <llvm/AsmParser/Parser.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>

namespace lb {

Parser::Parser() : local_slots(nullptr), global_slots(new llvm::SlotMapping()) {
  ;
}

std::tuple<std::unique_ptr<llvm::Module>, std::unique_ptr<llvm::MemoryBuffer>>
Parser::parse_ir(std::unique_ptr<llvm::MemoryBuffer> in,
                 llvm::LLVMContext& context) {
  g_message("Parsing IR");

  std::unique_ptr<llvm::Module> module;
  std::unique_ptr<llvm::MemoryBuffer> out;
  llvm::SMDiagnostic error;
  if(module = llvm::parseAssembly(in->getMemBufferRef(),
                                  error,
                                  context,
                                  global_slots.get())) {
    out = std::move(in);
    ir = out->getBuffer();
  } else {
    g_error("Error parsing IR: %s", error.getMessage().data());
  }
  
  return std::make_tuple(std::move(module), std::move(out));
}

std::tuple<std::unique_ptr<llvm::Module>, std::unique_ptr<llvm::MemoryBuffer>>
Parser::parse_bc(std::unique_ptr<llvm::MemoryBuffer> in,
                 llvm::LLVMContext& context) {
  g_message("Parsing bitcode");

  std::unique_ptr<llvm::Module> module;
  std::unique_ptr<llvm::MemoryBuffer> out;
  if(llvm::Expected<std::unique_ptr<llvm::Module>> expected
     = llvm::parseBitcodeFile(in->getMemBufferRef(), context)) {
    module = std::move(expected.get());
    g_message("Converting bitcode to IR");
    std::string s;
    llvm::raw_string_ostream ss(s);
    ss << *module;
    out = llvm::MemoryBuffer::getMemBufferCopy(ss.str(),
                                               in->getBufferIdentifier());
    ir = out->getBuffer();
  } else {
    g_error("Error parsing bitcode");
  }

  return std::make_tuple(std::move(module), std::move(out));
}

size_t
Parser::find(llvm::StringRef key, bool at_start, size_t cursor, bool wrap,
             std::set<size_t>& seen) {
  size_t found = ir.find(key, cursor);
  if(found == llvm::StringRef::npos) {
    if(wrap)
      return find(key, at_start, 0, false);
    return llvm::StringRef::npos;
  } else {
    if(at_start) {
      // If this was at the start of the line, then just return
      if(found > 0 and ir[found - 1] == '\n')
        return found;
      // If not, record that we have seen this match before and continue
      // searching. If we wrap around and come back to this position, then
      // we know to stop
      else if(seen.insert(found).second)
        return find(key, at_start, found + key.size(), wrap);
      else
        return llvm::StringRef::npos;
    } else {
      return found;
    }
  }
}

size_t
Parser::find(llvm::StringRef key, bool at_start, size_t cursor, bool wrap) {
  std::set<size_t> seen;
  return find(key, at_start, cursor, wrap, seen);
}

size_t
Parser::find_and_move(llvm::StringRef key, bool at_start, size_t& cursor) {
  size_t found = find(key, at_start, cursor, true);
  if(found != llvm::StringRef::npos)
    cursor = found + key.size();
  else
    g_error("Could not find key in IR: %s", key.data());
  return found;
}

size_t
Parser::find_and_move(const std::string& key, bool at_start, size_t& cursor) {
  return find_and_move(llvm::StringRef(key), at_start, cursor);
}

size_t
Parser::find_function(llvm::StringRef func,
                      llvm::StringRef prefix,
                      size_t& cursor,
                      std::set<size_t>& seen) {
  // Search for the prefix wrapping around if necessary
  size_t line_start = find(prefix, true, cursor, true);
  if(line_start != llvm::StringRef::npos) {
    // The only way we don't find a newline is if it is the last line
    size_t line_end   = find("\n", false, line_start + prefix.size(), false);
    if(line_end == llvm::StringRef::npos)
      line_end = find("\0", false, line_start + prefix.size(), false);
    llvm::StringRef substr = ir.substr(line_start, line_end);
    size_t found           = substr.find(func);
    if(found) {
      found = line_start + found;
      cursor = found + func.size();
      return found;
    } else if(seen.insert(found).second) {
      return find_function(func, prefix, cursor, seen);
    } else {
      llvm::errs() << "Could not find function in IR: " << func << "\n";
      g_error("Could not find function in IR: %s", func.data());
    }
  }
  return llvm::StringRef::npos;
}
  
size_t
Parser::find_function(llvm::StringRef func,
                      llvm::StringRef prefix,
                      size_t& cursor) {
  std::set<size_t> seen;
  return find_function(func, prefix, cursor, seen);
}

bool
Parser::link(lb::Module& module) {
  size_t cursor = 0;
  local_slots.reset(new llvm::ModuleSlotTracker(&module.get_llvm()));

  llvm::Module& llvm = module.get_llvm();

  g_message("Reading types");
  for(llvm::StructType* sty : llvm.getIdentifiedStructTypes()) {
    // TODO: At some point, we'll deal with unnamed struct types
    // but right now, I'm not sure how to get a handle to them in the IR
    if(sty->hasName()) {
      StructType& strct = module.add(sty);
      size_t pos        = find_and_move(strct.get_tag(), true, cursor);
      llvm::errs() << strct.get_tag() << " => " << pos << "\n";
    }
  }

  g_message("Reading global variables");
  for(llvm::GlobalVariable& global : llvm.globals()) {
    // TODO: At some point, we'll deal with unnamed globals. Right now,
    // we stick to named globals because we can definitely get a handle to
    // them in the LLVM IR
    if(global.hasName()) {
      GlobalVariable& g = module.add(global);
      size_t pos        = find_and_move(g.get_tag(), true, cursor);
      llvm::errs() << g.get_tag() << " => " << pos << "\n";
    }
  }

  // Do this in two passes because there may be circular references
  g_message("Reading functions");
  for(llvm::Function& function : llvm.functions()) {
    Function& f            = module.add(function);
    llvm::StringRef prefix = function.size() ? "define" : "declare";
    size_t pos = find_function(llvm::StringRef(f.get_tag()), prefix, cursor);
    llvm::errs() << f.get_tag() << " => " << pos << "\n";
  }

  g_message("Initializing global variables");
  for(llvm::GlobalVariable& global : llvm.globals()) {
    if(global.hasName()) {
      // module.get<GlobalVariable>(global).init(local_slots);
    }
  }

  g_message("Initializing functions");
  for(llvm::Function& function : llvm.functions())
    ;
  // module.get<Function>(function).init(local_slots);

  return true;
}

} // namespace lb

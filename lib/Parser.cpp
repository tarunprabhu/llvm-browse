#include "Parser.h"
#include "Argument.h"
#include "BasicBlock.h"
#include "Function.h"
#include "GlobalVariable.h"
#include "Instruction.h"

#include <glib.h>

#include <llvm/AsmParser/Parser.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>

using llvm::dyn_cast;
using llvm::isa;

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
Parser::find(llvm::StringRef key,
             size_t cursor,
             Lookback prev,
             bool wrap,
             std::set<size_t>& seen) {
  size_t found = ir.find(key, cursor);
  if(found == llvm::StringRef::npos) {
    if(wrap)
      return find(key, 0, prev, false, seen);
    return llvm::StringRef::npos;
  } else {
    bool discard = false;
    // Check the previous character if required
    switch(prev) {
    case Lookback::Any:
      return found;
      break;
    case Lookback::Newline:
      if(found > 0 and ir[found - 1] == '\n')
        return found;
      break;
    case Lookback::Whitespace:
      if(found > 0 and std::isspace(ir[found - 1]))
        return found;
      break;
    case Lookback::Indent:
      if(found > 0) {
        for(size_t i = 1; (not discard) and (ir[found - i] != '\n'); i++)
          if(!std::isspace(ir[found - i]))
            discard = true;
        if(not discard)
          return found;
      }
      break;
    default:
      g_critical("Internal error: Unknown Lookback type");
      break;
    }
    // If not, record that we have seen this match before and continue
    // searching. If we wrap around and come back to this position, then
    // we know to stop
    if(seen.insert(found).second)
      return find(key, found + key.size(), prev, wrap, seen);
    else
      return llvm::StringRef::npos;
  }
}

size_t
Parser::find(llvm::StringRef key, size_t cursor, Lookback prev, bool wrap) {
  std::set<size_t> seen;
  return find(key, cursor, prev, wrap, seen);
}

size_t
Parser::find_and_move(llvm::StringRef key, Lookback prev, size_t& cursor) {
  size_t found = find(key, cursor, prev, true);
  if(found != llvm::StringRef::npos)
    cursor = found + key.size();
  else
    g_error("Could not find key in IR: %s", key.data());
  return found;
}

size_t
Parser::find_and_move(const std::string& key, Lookback prev, size_t& cursor) {
  return find_and_move(llvm::StringRef(key), prev, cursor);
}

size_t
Parser::find_function(llvm::StringRef func,
                      llvm::StringRef prefix,
                      size_t& cursor,
                      std::set<size_t>& seen) {
  // Search for the prefix wrapping around if necessary
  size_t line_start = find(prefix, cursor, Lookback::Newline, true);
  if(line_start != llvm::StringRef::npos) {
    // The only way we don't find a newline is if it is the last line
    size_t line_end
        = find("\n", line_start + prefix.size(), Lookback::Any, false);
    if(line_end == llvm::StringRef::npos)
      line_end = find("\0", line_start + prefix.size(), Lookback::Any, false);
    llvm::StringRef substr = ir.substr(line_start, line_end);
    size_t found           = substr.find(func);
    if(found) {
      found  = line_start + found;
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
  BufferId id = Module::get_main_id();
  local_slots.reset(new llvm::ModuleSlotTracker(&module.get_llvm()));

  llvm::Module& llvm = module.get_llvm();

  g_message("Reading types");
  for(llvm::StructType* sty : llvm.getIdentifiedStructTypes()) {
    // TODO: At some point, we'll deal with unnamed struct types
    // but right now, I'm not sure how to get a handle to them in the IR
    if(sty->hasName()) {
      StructType& strct = module.add(sty);
      size_t pos = find_and_move(strct.get_tag(), Lookback::Newline, cursor);
      strct.set_defn_range(SourceRange(id, pos, pos + strct.get_tag().size()));
    }
  }

  g_message("Reading global variables");
  for(llvm::GlobalVariable& global : llvm.globals()) {
    // TODO: At some point, we'll deal with unnamed globals. Right now,
    // we stick to named globals because we can definitely get a handle to
    // them in the LLVM IR
    if(global.hasName()) {
      GlobalVariable& g = module.add(global);
      size_t pos        = find_and_move(g.get_tag(), Lookback::Newline, cursor);
      g.set_defn_range(SourceRange(id, pos, pos + g.get_tag().size()));
    }
  }

  // Do this in two passes because there may be circular references
  g_message("Reading functions");
  for(llvm::Function& function : llvm.functions()) {
    Function& f            = module.add(function);
    llvm::StringRef prefix = function.size() ? "define" : "declare";
    size_t pos = find_function(llvm::StringRef(f.get_tag()), prefix, cursor);
    f.set_defn_range(SourceRange(id, pos, pos + f.get_tag().size()));
  }

  g_message("Initializing global variables");
  for(llvm::GlobalVariable& global : llvm.globals()) {
    if(global.hasName()) {
      // FIXME: If the global has an initializer, we need to parse it to
      // find the uses of other constants within it. An example use case is
      // the vtables which will definitely use the methods of a class.
    }
  }

  // Not really sure if this is actually helping.
  // But the idea is that I don't have to allocate a string every time
  // I need to create a key to search for an instruction definition
  std::string buf;
  llvm::raw_string_ostream ss(buf);
  
  g_message("Initializing functions");
  for(llvm::Function& llvm_f : llvm.functions()) {
    // For functions that are declared, we don't even try to do anything
    if(not llvm_f.size())
      continue;

    Function& f = module.get(llvm_f);
    // Reposition the cursor at the function definition because we know that
    // things will  be closer
    cursor         = f.get_defn_range().get_end();
    size_t f_begin = find_and_move(llvm::StringRef("{"), Lookback::Any, cursor);

    llvm::errs() << llvm_f.getName() << "\n";
    local_slots->incorporateFunction(llvm_f);
    for(llvm::Argument& llvm_arg : llvm_f.args()) {
      Argument& arg = module.get(llvm_arg);
      if(llvm_arg.hasName())
        arg.set_tag(llvm_arg.getName(), "%");
      else
        arg.set_tag(local_slots->getLocalSlot(&llvm_arg));

      // Not going to try and set a definition for arguments. Currently, LLVM
      // removes all references to them in the IR. Even defined functions
      // only have a type and not even a slot representation for the arguments.
      // Of course, they implicitly show up in the code afterwards which
      // is really nice! The reasoning is so the IR is smaller. I am not sure
      // how much smaller the IR becomes as a result of these elisions and how
      // much of a benefit is derived from it. I really hope it is significant,
      // otherwise, it's yet another one of those micro-optimizations that
      // ends up being a pain in the ass for some people.
    }

    // Iterate over all the basic blocks first and set their tag because
    // we will have "forward references" to them in branch instructions
    // and if we don't assign them a tag first, we can't link them up
    // correctly
    for(llvm::BasicBlock& llvm_bb : llvm_f) {
      BasicBlock& bb = module.get(llvm_bb);
      if(llvm_bb.hasName())
        bb.set_tag(llvm_bb.getName(), "%");
      else
        bb.set_tag(local_slots->getLocalSlot(&llvm_bb));
    }

    // Now iterate over all the basic blocks and the instructions
    // We don't have to worry about forward iterations on instructions because
    // it is incorrect to have an instruction use preceding a definition
    for(llvm::BasicBlock& llvm_bb : llvm_f) {
      BasicBlock& bb = module.get(llvm_bb);
      for(const llvm::Instruction& llvm_inst : llvm_bb) {
        Instruction& inst = module.get(llvm_inst);
        if(llvm_inst.hasName())
          inst.set_tag(llvm_inst.getName(), "%");
        else if(not llvm_inst.getType()->isVoidTy())
          inst.set_tag(local_slots->getLocalSlot(&llvm_inst));
        else if(const auto* call = dyn_cast<llvm::CallInst>(&llvm_inst))
          if(call->isTailCall())
            inst.set_tag("tail call");
          else
            inst.set_tag("call");
        else
          inst.set_tag(llvm_inst.getOpcodeName());

        buf.clear();
        ss << inst.get_tag();
        if(not llvm_inst.getType()->isVoidTy())
          ss << " =";

        size_t pos = find_and_move(ss.str(), Lookback::Whitespace, cursor);
        inst.set_defn_range(SourceRange(id, pos, pos + inst.get_tag().size()));

        llvm::errs() << "  " << inst.get_tag() << "\n";
        for(const llvm::Value* op : llvm_inst.operand_values()) {
          if(module.contains(*op))
            llvm::errs() << "    " << module.get(*op).get_tag() << " => '"
                         << op->hasName() << "'\n";
          else
            llvm::errs() << "    <null>\n";
        }
      }

      // There isn't a reasonable way to find the start of a basic block
      // other than by finding the location of the first instruction in it
      // It might not be safe to rely on the labels being printed as comments
      // Already, the label for the entry block has been removed from the IR
      size_t bb_begin
          = module.get(llvm_bb.front()).get_defn_range().get_begin();

      // Similarly, the end of the block is a bit problematic because
      // instructions can span multiple lines and relying on any particular
      // representation of the instruction is a bad idea.
      // If this is not the exit block, once we have the last instruction,
      // we continue looking for the first blank line because there is always
      // an empty line between basic blocks (hopefully that won't go away)
      // If it is the last basic block in the function, then look for the
      // closing brace because that indicates the end of the function
      size_t bb_end = llvm::StringRef::npos;
      if(&llvm_bb != &llvm_f.back()) {
        bb_end
            = find_and_move(llvm::StringRef("\n"), Lookback::Newline, cursor);
      } else {
        bb_end = find_and_move(llvm::StringRef("}"), Lookback::Newline, cursor);
        // Move it so the block ends *before* the closing brace. We want the
        // function to end at the brace. Tiny thing, but still
        if(bb_end != llvm::StringRef::npos)
          bb_end -= 1;
      }

      if(bb_end != llvm::StringRef::npos)
        bb.set_defn_range(SourceRange(id, bb_begin, bb_end));
    }

    size_t f_end = module.get(llvm_f.back()).get_defn_range().get_end();
    if(f_end != llvm::StringRef::npos)
      f.set_defn_range(SourceRange(id, f_begin, f_end + 1));
  }

  return true;
}

} // namespace lb

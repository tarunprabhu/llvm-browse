#include "Parser.h"
#include "Argument.h"
#include "BasicBlock.h"
#include "Function.h"
#include "GlobalAlias.h"
#include "GlobalVariable.h"
#include "INavigable.h"
#include "Instruction.h"
#include "Logging.h"
#include "MDNode.h"
#include "Module.h"
#include "Value.h"

#include <llvm/AsmParser/Parser.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Metadata.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>

using llvm::cast;
using llvm::dyn_cast;
using llvm::dyn_cast_or_null;
using llvm::isa;

namespace lb {

template<typename T>
static void
sort_by_tag_name(std::vector<T*>& objs) {
  std::sort(objs.begin(), objs.end(), [](const T* l, const T* r) {
    return l->get_tag().size() < r->get_tag().size();
  });
  std::reverse(objs.begin(), objs.end());
}

Parser::Parser() : local_slots(nullptr), global_slots(nullptr) {
  ;
}

std::tuple<std::unique_ptr<llvm::Module>, std::unique_ptr<llvm::MemoryBuffer>>
Parser::parse_ir(std::unique_ptr<llvm::MemoryBuffer> in,
                 llvm::LLVMContext& context) {
  message() << "Parsing IR\n";

  global_slots.reset(new llvm::SlotMapping());
  std::unique_ptr<llvm::Module> module;
  std::unique_ptr<llvm::MemoryBuffer> out;
  llvm::SMDiagnostic error;
  if((module = llvm::parseAssembly(
          in->getMemBufferRef(), error, context, global_slots.get()))) {
    out = std::move(in);
    ir  = out->getBuffer();
  } else {
    critical() << "Error parsing IR: " << error.getMessage() << "\n";
  }

  return std::make_tuple(std::move(module), std::move(out));
}

std::tuple<std::unique_ptr<llvm::Module>, std::unique_ptr<llvm::MemoryBuffer>>
Parser::parse_bc(std::unique_ptr<llvm::MemoryBuffer> in,
                 llvm::LLVMContext& context) {
  message() << "Parsing bitcode\n";

  // When parsing a bitcode, we convert it back to IR and parse the IR instead.
  // It's the only way to get the slot numbers for metadata nodes
  // so we can link them up as well. At some point, we may make the linked
  // metadata step optional, in which case we won't have to do this silly
  // double parsing
  //
  llvm::LLVMContext tmp;
  std::unique_ptr<llvm::Module> module;
  std::unique_ptr<llvm::MemoryBuffer> out;
  if(llvm::Expected<std::unique_ptr<llvm::Module>> expected
     = llvm::parseBitcodeFile(in->getMemBufferRef(), tmp)) {
    message() << "Converting bitcode to IR\n";
    std::string s;
    llvm::raw_string_ostream ss(s);
    ss << *expected.get();
    return parse_ir(llvm::MemoryBuffer::getMemBufferCopy(llvm::StringRef(s)),
                    context);
  } else {
    critical() << "Error parsing bitcode\n";
  }

  return std::make_tuple(std::move(module), std::move(out));
}

Offset
Parser::find(llvm::StringRef key,
             Offset cursor,
             Lookback prev,
             bool wrap,
             std::set<Offset>& seen) {
  Offset found = ir.find(key, cursor);
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
        for(Offset i = 1; (not discard) and (ir[found - i] != '\n'); i++)
          if(!std::isspace(ir[found - i]))
            discard = true;
        if(not discard)
          return found;
      }
      break;
    default:
      error() << "Internal error: Unknown Lookback type\n";
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

Offset
Parser::find(llvm::StringRef key, Offset cursor, Lookback prev, bool wrap) {
  std::set<Offset> seen;
  return find(key, cursor, prev, wrap, seen);
}

Offset
Parser::find_and_move(llvm::StringRef key, Lookback prev, Offset& cursor) {
  Offset found = find(key, cursor, prev, true);
  if(found != llvm::StringRef::npos)
    cursor = found + key.size();
  else
    critical() << "Could not find key in IR: " << key << "\n";
  return found;
}

Offset
Parser::find_and_move(const std::string& key, Lookback prev, Offset& cursor) {
  return find_and_move(llvm::StringRef(key), prev, cursor);
}

Offset
Parser::find_function(llvm::StringRef func,
                      llvm::StringRef prefix,
                      Offset& cursor,
                      std::set<Offset>& seen) {
  // Search for the prefix wrapping around if necessary
  Offset line_start = find(prefix, cursor, Lookback::Newline, true);
  if(line_start != llvm::StringRef::npos) {
    // The only way we don't find a newline is if it is the last line
    Offset line_end
        = find("\n", line_start + prefix.size(), Lookback::Any, false);
    if(line_end == llvm::StringRef::npos)
      line_end = find("\0", line_start + prefix.size(), Lookback::Any, false);
    llvm::StringRef substr = ir.substr(line_start, line_end - line_start);
    Offset found           = substr.find(func);
    if(found != llvm::StringRef::npos) {
      found  = line_start + found;
      cursor = found + func.size();
      return found;
    } else if(seen.insert(found).second) {
      return find_function(func, prefix, cursor, seen);
    } else {
      critical() << "Could not find function in IR: " << func << "\n";
    }
  }
  return llvm::StringRef::npos;
}

Offset
Parser::find_function(llvm::StringRef func,
                      llvm::StringRef prefix,
                      Offset& cursor) {
  std::set<Offset> seen;
  return find_function(func, prefix, cursor, seen);
}

void
Parser::collect_constants(const llvm::Constant* c,
                          Module& module,
                          std::vector<INavigable*>& consts) {
  if(const auto* cexpr = dyn_cast<llvm::ConstantExpr>(c)) {
    for(const llvm::Value* op : cexpr->operand_values())
      collect_constants(cast<llvm::Constant>(op), module, consts);
  } else if(const auto* cstruct = dyn_cast<llvm::ConstantStruct>(c)) {
    for(const llvm::Value* op : cstruct->operand_values())
      collect_constants(cast<llvm::Constant>(op), module, consts);
  } else if(const auto* carray = dyn_cast<llvm::ConstantArray>(c)) {
    llvm::Type* ty = carray->getType()->getElementType();
    if(not(ty->isIntegerTy() or ty->isFloatingPointTy() or ty->isHalfTy()))
      for(const llvm::Value* op : carray->operand_values())
        collect_constants(cast<llvm::Constant>(op), module, consts);
  } else if(const auto* g = dyn_cast<llvm::GlobalVariable>(c)) {
    consts.push_back(static_cast<INavigable*>(&module.get(*g)));
  } else if(const auto* a = dyn_cast<llvm::GlobalAlias>(c)) {
    consts.push_back(static_cast<INavigable*>(&module.get(*a)));
  } else if(const auto* f = dyn_cast<llvm::Function>(c)) {
    consts.push_back(static_cast<INavigable*>(&module.get(*f)));
  } else if(isa<llvm::ConstantData>(c) or isa<llvm::BlockAddress>(c)) {
    ;
  } else {
    critical() << "Unknown constant type: " << *c << "\n";
  }
}

std::vector<INavigable*>
Parser::collect_constants(const llvm::Constant* c, Module& module) {
  std::vector<INavigable*> consts;
  collect_constants(c, module, consts);
  return consts;
}

bool
Parser::overlaps(Offset pos, const std::map<INavigable*, Offset>& mapped) {
  for(const auto& it : mapped) {
    const INavigable* v = it.first;
    Offset begin        = it.second;
    Offset end          = begin + v->get_tag().size();
    if(pos >= begin and pos <= end)
      return true;
  }
  return false;
}

std::map<INavigable*, Offset>
Parser::associate_values(std::vector<INavigable*> values,
                         Module& module,
                         Offset cursor,
                         Instruction* inst) {
  // The list of values provided here are typically the operands in a
  // LLVM::Instruction or llvm::ConstantExpr. We need to find the corresponding
  // tag for each value in the text and associate the value with that piece
  // of text

  sort_by_tag_name(values);
  std::map<INavigable*, Offset> mapped;
  for(INavigable* v : values) {
    llvm::StringRef tag = v->get_tag();
    Offset pos          = find(tag, cursor, Lookback::Whitespace, false);
    while(overlaps(pos, mapped))
      pos = find(tag, pos + 1, Lookback::Whitespace, false);
    mapped[v] = pos;
    v->add_use(Use::make(pos, pos + v->get_tag().size(), *v, module, inst));
  }

  return mapped;
}

static bool
is_debug_metadata(const llvm::MDNode* md) {
  return isa<llvm::DINode>(md) or isa<llvm::DILocation>(md)
         or isa<llvm::DIExpression>(md)
         or isa<llvm::DIGlobalVariableExpression>(md);
}

std::vector<const llvm::MDNode*>
Parser::get_metadata(const llvm::GlobalObject& g) {
  std::vector<const llvm::MDNode*> ret;
  llvm::SmallVector<std::pair<unsigned, llvm::MDNode*>, 8> mds;
  g.getAllMetadata(mds);
  for(const auto& i : mds) {
    const llvm::MDNode* md = i.second;
    if(not is_debug_metadata(md))
      ret.push_back(md);
  }
  return ret;
}

std::vector<const llvm::MDNode*>
Parser::get_metadata(const llvm::Instruction& inst) {
  std::vector<const llvm::MDNode*> ret;
  llvm::SmallVector<std::pair<unsigned, llvm::MDNode*>, 8> mds;
  inst.getAllMetadataOtherThanDebugLoc(mds);
  for(const auto& i : mds) {
    const llvm::MDNode* md = i.second;
    if(not is_debug_metadata(md))
      ret.push_back(md);
  }
  return ret;
}

bool
Parser::link(Module& module) {
  // Not really sure if this is actually helping.
  // But the idea is that I don't have to allocate a string every time
  // I need to create a key to search for an instruction definition.
  // Same for the vector for metadata nodes
  std::string buf;
  llvm::raw_string_ostream ss(buf);
  std::vector<INavigable*> ops;
  std::set<const llvm::MDNode*> wl;
  Offset cursor = 0;

  local_slots.reset(new llvm::ModuleSlotTracker(&module.get_llvm()));

  llvm::Module& llvm = module.get_llvm();

  // The next steps are done in order for a reason
  // As of LLVM 8, the StructType's appear first in the IR file followed by the
  // Comdat's, GlobalVariable's, GlobalAlias'es and the Function's.
  // We don't want to have to keep jumping back and forth for things in the
  // text so we process everything in order

  message() << "Reading types\n";
  for(llvm::StructType* llvm_sty : llvm.getIdentifiedStructTypes()) {
    // TODO: At some point, we'll deal with unnamed struct types
    // but right now, I'm not sure how to get a handle to them in the IR
    if(llvm_sty->hasName()) {
      StructType& sty = StructType::make(llvm_sty, module);
      Offset pos      = find_and_move(sty.get_tag(), Lookback::Newline, cursor);
      sty.set_llvm_defn(
          Definition::make(pos, pos + sty.get_tag().size(), sty, module));
    } else {
      warning() << "Skipping unnamed struct type: " << llvm_sty << "\n";
    }
  }

  // The comdats are unusual because what looks like a "definition" in the LLVM
  // IR, we will treat as an implicit use and attach a definition to it.
  // This definition will be the same as the function/global that the
  // Comdat represents
  std::map<llvm::Function*, Offset> comdats;
  message() << "Reading comdats\n";
  for(llvm::Function& f : llvm.functions()) {
    if(llvm::Comdat* llvm_c = f.getComdat()) {
      Comdat& comdat  = Comdat::make(*llvm_c, f, module);
      Offset pos = find_and_move(comdat.get_tag(), Lookback::Newline, cursor);
      if(pos == llvm::StringRef::npos)
        critical() << "Could not find comdat definition: " << comdat.get_tag()
                   << "\n";
      else
        comdat.set_self_llvm_defn(
            LLVMRange(pos, pos + comdat.get_tag().size()));
    }
  }
  for(llvm::GlobalVariable& g : llvm.globals()) {
    if(llvm::Comdat* llvm_c = g.getComdat()) {
      Comdat& comdat  = Comdat::make(*llvm_c, g, module);
      Offset pos = find_and_move(comdat.get_tag(), Lookback::Newline, cursor);
      if(pos == llvm::StringRef::npos)
        critical() << "Could not find comdat definition: " << comdat.get_tag()
                   << "\n";
      else
        comdat.set_self_llvm_defn(
            LLVMRange(pos, pos + comdat.get_tag().size()));
    }
  }

  message() << "Reading global variables\n";
  for(llvm::GlobalVariable& llvm_g : llvm.globals()) {
    // TODO: At some point, we'll deal with unnamed globals. Right now,
    // we stick to named globals because we can definitely get a handle to
    // them in the LLVM IR
    if(llvm_g.hasName()) {
      GlobalVariable& g = GlobalVariable::make(llvm_g, module);
      Offset pos        = find_and_move(g.get_tag(), Lookback::Newline, cursor);
      if(pos == llvm::StringRef::npos) {
        critical() << "Could not find global definition: " << g.get_tag()
                   << "\n";
      } else {
        Offset end = pos + g.get_tag().size();
        Definition& def = Definition::make(pos, end, g, module);
        g.set_llvm_defn(def);
        if(llvm::Comdat* c = llvm_g.getComdat())
          module.get(*c).set_llvm_defn(def);
      }

      // FIXME: Skipping any metadata on global variables because I can't
      // figure out how to get just the non-debug
      // for(const llvm::MDNode* md : get_metadata(llvm_g))
      //   wl.insert(md);
    } else {
      warning() << "Skipping unnamed global: " << llvm_g << "\n";
    }
  }

  message() << "Reading global aliases\n";
  for(llvm::GlobalAlias& llvm_a : llvm.aliases()) {
    GlobalAlias& a = GlobalAlias::make(llvm_a, module);
    Offset pos     = find_and_move(a.get_tag(), Lookback::Newline, cursor);
    if(pos == llvm::StringRef::npos)
      critical() << "Could not find alias definition: " << a.get_tag() << "\n";
    else
      a.set_llvm_defn(
          Definition::make(pos, pos + a.get_tag().size(), a, module));
  }

  // Do this in two passes because there may be circular references
  message() << "Reading functions\n";
  for(llvm::Function& llvm_f : llvm.functions()) {
    Function& f            = Function::make(llvm_f, module);
    llvm::StringRef prefix = llvm_f.size() ? "define" : "declare";
    Offset pos             = find_function(f.get_tag(), prefix, cursor);
    if(pos == llvm::StringRef::npos) {
      critical() << "Could not find function definition: " << f.get_tag()
                 << "\n";
    } else {
      Offset end = pos + f.get_tag().size();
      Definition& def = Definition::make(pos, end, f, module);
      f.set_llvm_defn(def);
      if(llvm::Comdat* c = llvm_f.getComdat())
        module.get(*c).set_llvm_defn(def);
    }
    for(const llvm::MDNode* md : get_metadata(llvm_f))
      wl.insert(md);
  }

  message() << "Reading metadata\n";
  for(const auto& i : global_slots->MetadataNodes) {
    MDNode& md = MDNode::make(*i.second, i.first, module);
    buf.clear();
    ss << md.get_tag() << " =";
    Offset pos = find_and_move(ss.str(), Lookback::Newline, cursor);
    if(pos == llvm::StringRef::npos)
      critical() << "Could not find metadata definition: " << md.get_tag()
                 << "\n";
    else
      md.set_llvm_defn(
          Definition::make(pos, pos + md.get_tag().size(), md, module));
  }

  message() << "Processing global variables\n";
  for(llvm::GlobalVariable& llvm_g : llvm.globals()) {
    // The global might not be in the module if it doesn't have a name
    if(module.contains(llvm_g)) {
      GlobalVariable& g = module.get(llvm_g);
      if(llvm_g.hasInitializer())
        associate_values(collect_constants(llvm_g.getInitializer(), module),
                         module,
                         g.get_llvm_defn().get_end());
    }
  }

  message() << "Processing functions\n";
  for(llvm::Function& llvm_f : llvm.functions()) {
    // For functions that are declared, we don't even try to do anything
    if(not llvm_f.size())
      continue;

    Function& f = module.get(llvm_f);
    // Reposition the cursor at the function definition because we know that
    // things will  be closer
    cursor = f.get_llvm_defn().get_end();
    local_slots->incorporateFunction(llvm_f);
    Offset f_begin = find_and_move(llvm::StringRef("{"), Lookback::Any, cursor);
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

    // Iterate over all the basic blocks and instructions and set their tag
    // first because we can have "forward references" to them in branch and phi
    // instructions respectively. If we don't assign them a tag first,
    // we can't link them up correctly
    for(llvm::BasicBlock& llvm_bb : llvm_f) {
      BasicBlock& bb = module.get(llvm_bb);
      if(llvm_bb.hasName())
        bb.set_tag(llvm_bb.getName(), "%");
      else
        bb.set_tag(local_slots->getLocalSlot(&llvm_bb));
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
      }
    }

    // Now iterate over all the basic blocks and the instructions
    // We don't have to worry about forward iterations on instructions because
    // it is incorrect to have an instruction use preceding a definition
    for(llvm::BasicBlock& llvm_bb : llvm_f) {
      BasicBlock& bb = module.get(llvm_bb);
      Instruction* inst_prev = nullptr;
      for(const llvm::Instruction& llvm_inst : llvm_bb) {
        Instruction& inst   = module.get(llvm_inst);
        llvm::StringRef tag = inst.get_tag();
        buf.clear();
        ss << tag;
        if(not llvm_inst.getType()->isVoidTy())
          ss << " =";

        Offset i_begin = find_and_move(ss.str(), Lookback::Whitespace, cursor);
        if(llvm_inst.getType()->isVoidTy())
          inst.set_llvm_defn(Definition::make(i_begin, i_begin, inst, module));
        else
          inst.set_llvm_defn(
              Definition::make(i_begin, i_begin + tag.size(), inst, module));

        // Because we don't want to even try to parse the instruction operands,
        // everything will have to be text-based matching. To reduce the
        // possibility of false matches, the operands that have to linked
        // are first sorted by length and matched from the longest to the
        // shortest. This way, if a shorter operand which happens to be a
        // substring of a longer one matches against a previous match, it can
        // be ignored. This does not care if the instruction is split over
        // multiple lines.
        ops.clear();
        for(const llvm::Value* op : llvm_inst.operand_values())
          // If the module does not contain the operand, then it is either a
          // llvm::Metadata (more specifically, llvm::MetadataAsValue) or
          // an llvm::Constant. Most constants we don't care about, but
          // we do care about llvm::ConstantExpr because they could contain
          // references to llvm::Function or llvm::GlobalVariable that we
          // do care about. As with other instances, we just collect them
          // all now and process them later
          if(const auto* i = dyn_cast<llvm::Instruction>(op))
            ops.push_back(&module.get(*i));
          else if(const auto* a = dyn_cast<llvm::Argument>(op))
            ops.push_back(&module.get(*a));
          else if(const auto* f = dyn_cast<llvm::Function>(op))
            ops.push_back(&module.get(*f));
          else if(const auto* g = dyn_cast<llvm::GlobalVariable>(op))
            ops.push_back(&module.get(*g));
          else if(const auto* a = dyn_cast<llvm::GlobalAlias>(op))
            ops.push_back(&module.get(*a));
          else if(const auto* bb = dyn_cast<llvm::BasicBlock>(op))
            ops.push_back(&module.get(*bb));
          else if(const auto* c = dyn_cast<llvm::Constant>(op))
            collect_constants(c, module, ops);
          else if(isa<llvm::MetadataAsValue>(op))
            ;
          else
            critical() << "Unexpected instruction operand: " << *op << "\n";

        for(const llvm::MDNode* llvm_md : get_metadata(llvm_inst)) {
          ops.push_back(&module.get(*llvm_md));
          // This will be used to collect all the MDNode's seen in function
          // metadata after which it will get used to get all MDNodes reachable
          // from it
          wl.insert(llvm_md);
        }

        std::map<INavigable*, Offset> mapped
            = associate_values(std::move(ops), module, i_begin, &inst);

        // Because instructions can span multiple lines, a reasonable way to 
        // determine the span of an instruction is to wait until the next
        // instruction in the block is found and assume that it extends till 
        // the end of the nearest non-empty line prior to the current 
        // instruction. The last instruction in the basic block will be 
        // dealt with when the span of the basic block is computed because it 
        // will be assumed to span till the end of the block
        if(inst_prev) {
          Offset end = cursor;
          while(ir[end] != '\n')
            end--;
          inst_prev->set_llvm_span(
              LLVMRange(inst_prev->get_llvm_defn().get_begin(), end));
        }
        inst_prev = &inst;
      }

      // There isn't a reasonable way to find the start of a basic block
      // other than by finding the location of the first instruction in it
      // It might not be safe to rely on the labels being printed as comments
      // Already, the label for the entry block has been removed from the IR
      // We don't really have a reasonable place to go to when we go to the
      // definition of a basic block other than to the start of the first
      // instruction
      Offset bb_begin = module.get(llvm_bb.front()).get_llvm_defn().get_begin();
      bb.set_llvm_defn(Definition::make(bb_begin, bb_begin, bb, module));

      // Similarly, the end of the block is a bit problematic because
      // instructions can span multiple lines and relying on any particular
      // representation of the instruction is a bad idea.
      // If this is not the exit block, once we have the last instruction,
      // we continue looking for the first blank line because there is always
      // an empty line between basic blocks (hopefully that won't go away)
      // If it is the last basic block in the function, then look for the
      // closing brace because that indicates the end of the function
      Offset bb_end = llvm::StringRef::npos;
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

      if(bb_end != llvm::StringRef::npos) {
        bb.set_llvm_span(LLVMRange(bb_begin, bb_end));
        if(inst_prev)
          inst_prev->set_llvm_span(
              LLVMRange(inst_prev->get_llvm_defn().get_begin(), bb_end));
      } else {
        warning() << "Could not compute span for basic block\n";
      }
    }

    Offset f_end = module.get(llvm_f.back()).get_llvm_span().get_end();
    if(f_end != llvm::StringRef::npos)
      f.set_llvm_span(LLVMRange(f_begin, f_end + 1));
    else
      warning() << "Could not compute span for function: "
                << f.get_llvm().getName() << "\n";
  }

  message() << "Processing metadata\n";
  // Add MDNodes reachable from NamedMDNodes
  for(const llvm::NamedMDNode& nmd : llvm.named_metadata())
    for(const llvm::MDNode* md : nmd.operands())
      if(not is_debug_metadata(md))
        wl.insert(md);

  // Expand the number of MDNodes that get used until we reach a fixed point
  // The DI* nodes should not be navigable, but everything else should
  std::set<const llvm::MDNode*> seen;
  for(const llvm::MDNode* md : wl)
    seen.insert(md);
  std::set<const llvm::MDNode*> wl2;
  do {
    for(const llvm::MDNode* md : wl)
      for(const llvm::MDOperand& mop : md->operands())
        if(const auto* op = dyn_cast_or_null<llvm::MDNode>(mop))
          if(not is_debug_metadata(op))
            if(seen.insert(op).second)
              wl2.insert(op);
    wl = std::move(wl2);
  } while(wl.size());

  std::vector<MDNode*> mds;
  for(const llvm::MDNode* md : seen)
    if(module.contains(*md))
      mds.push_back(&module.get(*md));
  sort_by_tag_name(mds);
  for(MDNode* md : mds) {
    const llvm::MDNode& llvm_md = md->get_llvm();
    cursor                      = md->get_llvm_defn().get_end();
    for(unsigned i = 0; i < llvm_md.getNumOperands(); i++) {
      if(const auto* mop
         = dyn_cast_or_null<llvm::MDNode>(llvm_md.getOperand(i))) {
        MDNode& op = module.get(*mop);
        buf.clear();
        ss << op.get_tag();
        if(i < llvm_md.getNumOperands() - 1)
          ss << ",";
        else
          ss << "}";
        Offset pos = find_and_move(ss.str(), Lookback::Any, cursor);
        if(pos != llvm::StringRef::npos)
          op.add_use(Use::make(pos, pos + op.get_tag().size(), op, module));
        else
          warning() << "Could not find metadata operand: " << op.get_tag()
                    << "\n";
      }
    }
  }

  message() << "Done parsing IR\n";

  return true;
}

} // namespace lb

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "lib/Argument.h"
#include "lib/BasicBlock.h"
#include "lib/Function.h"
#include "lib/GlobalAlias.h"
#include "lib/GlobalVariable.h"
#include "lib/Instruction.h"
#include "lib/Logging.h"
#include "lib/MDNode.h"
#include "lib/Module.h"
#include "lib/StructType.h"

#include <llvm/ADT/iterator_range.h>

#include <cstdio>
#include <type_traits>

using Handle = uint64_t;

// Invalid handle
static const int HANDLE_NULL = 0;

// The tags are added to the Handle to be able to determine the dynamic type
// of the object from the handle
enum class HandleKind {
  Module         = 0x1,
  GlobalAlias    = 0x2,
  Argument       = 0x3,
  BasicBlock     = 0x4,
  Comdat         = 0x5,
  Function       = 0x6,
  GlobalVariable = 0x7,
  Instruction    = 0x8,
  MDNode         = 0x9,
  StructType     = 0xA,
  Mask           = 0xf,
};

template<typename T, std::enable_if_t<!std::is_pointer<T>::value, int> = 0>
static Handle
get_handle(const T* ptr, HandleKind tag) {
  return reinterpret_cast<Handle>(ptr) | static_cast<Handle>(tag);
}

template<typename T, std::enable_if_t<!std::is_pointer<T>::value, int> = 0>
static PyObject*
get_py_handle(const T* ptr, HandleKind tag) {
  return PyLong_FromLong(get_handle(ptr, tag));
}

template<typename T>
static T*
get_object(Handle handle) {
  return reinterpret_cast<T*>(handle & ~static_cast<Handle>(HandleKind::Mask));
}

static HandleKind
get_handle_kind(Handle handle) {
  return static_cast<HandleKind>(handle
                                 & static_cast<Handle>(HandleKind::Mask));
}

static std::string
get_handle_kind_name(Handle handle) {
  switch(get_handle_kind(handle)) {
  case HandleKind::GlobalAlias:
    return "Alias";
  case HandleKind::Argument:
    return "Argument";
  case HandleKind::BasicBlock:
    return "Basic Block";
  case HandleKind::Comdat:
    return "Comdat";
  case HandleKind::Function:
    return "Function";
  case HandleKind::GlobalVariable:
    return "Global Variable";
  case HandleKind::Instruction:
    return "Instruction";
  case HandleKind::MDNode:
    return "Metadata";
  case HandleKind::Module:
    return "Module";
  case HandleKind::StructType:
    return "Struct";
  default:
    return "<<UNKNOWN>>";
  }
}

static Handle
parse_handle(PyObject* args) {
  Handle handle = HANDLE_NULL;
  if(!PyArg_ParseTuple(args, "k", &handle))
    return HANDLE_NULL;
  return handle;
}

static PyStructSequence_Field PySourcePointFields[] = {
    {"line", nullptr},
    {"column", nullptr},
    {nullptr, nullptr},
};

static PyStructSequence_Desc PySourcePointDesc = {
    "SourcePoint",
    "A tuple of line number and column number",
    PySourcePointFields,
    2,
};

static PyStructSequence_Field PySourceRangeFields[] = {
    {"file", "Full path to the source file"},
    {"begin", "Tuple of (line, col) for the start of the range"},
    {"end", "Tuple of (line, col) for the end of the range"},
    {nullptr, nullptr},
};

static PyStructSequence_Desc PySourceRangeDesc = {
    "SourceRange",
    "Represents a range in the source file",
    PySourceRangeFields,
    3,
};

static PyStructSequence_Field PyLLVMRangeFields[] = {
    {"begin", "Offset in the LLVM IR file to the start of the range"},
    {"end", "Offset in the LLVM IR file to the end of the range"},
    {nullptr, nullptr},
};

static PyStructSequence_Desc PyLLVMRangeDesc = {
    "LLVMRange",
    "A range of offsets in the LLVM IR file",
    PyLLVMRangeFields,
    2,
};

// These will be created when the module is initialized
static PyTypeObject* PySourcePoint = nullptr;
static PyTypeObject* PySourceRange = nullptr;
static PyTypeObject* PyLLVMRange   = nullptr;

static PyObject*
convert(bool b) {
  return PyBool_FromLong(b);
}

static PyObject*
convert(llvm::StringRef s) {
  if(s.size())
    return PyUnicode_FromString(s.data());
  return PyUnicode_FromString("");
}

static PyObject*
convert(const lb::SourceRange& range) {
  PyObject* py = Py_None;
  if(range) {
    PyObject* py_begin = PyStructSequence_New(PySourcePoint);
    PyStructSequence_SetItem(py_begin, 0, PyLong_FromLong(range.begin.line));
    PyStructSequence_SetItem(py_begin, 1, PyLong_FromLong(range.begin.column));
    Py_INCREF(py_begin);

    PyObject* py_end = PyStructSequence_New(PySourcePoint);
    PyStructSequence_SetItem(py_end, 0, PyLong_FromLong(range.end.line));
    PyStructSequence_SetItem(py_end, 1, PyLong_FromLong(range.end.column));
    Py_INCREF(py_end);

    py = PyStructSequence_New(PySourceRange);
    if(range.file)
      PyStructSequence_SetItem(py, 0, PyUnicode_FromString(range.file));
    else
      PyStructSequence_SetItem(py, 0, PyUnicode_FromString(""));
    PyStructSequence_SetItem(py, 1, py_begin);
    PyStructSequence_SetItem(py, 2, py_end);
  }

  Py_INCREF(py);
  return py;
}

static PyObject*
convert(const lb::LLVMRange& range) {
  PyObject* py = Py_None;
  if(range) {
    py = PyStructSequence_New(PyLLVMRange);
    PyStructSequence_SetItem(py, 0, PyLong_FromLong(range.begin));
    PyStructSequence_SetItem(py, 1, PyLong_FromLong(range.end));
  }

  Py_INCREF(py);
  return py;
}

static PyObject*
convert(llvm::iterator_range<lb::INavigable::Iterator> uses) {
  PyObject* list = PyList_New(0);
  for(const lb::LLVMRange& range : uses)
    PyList_Append(list, convert(range));

  Py_INCREF(list);
  return list;
}

// PUBLIC methods

// Utils

static PyObject*
get_null_handle(PyObject* self, PyObject* args) {
  return PyLong_FromLong(HANDLE_NULL);
}

static PyObject*
is_alias(PyObject* self, PyObject* args) {
  return convert(get_handle_kind(parse_handle(args))
                 == HandleKind::GlobalAlias);
}

static PyObject*
is_argument(PyObject* self, PyObject* args) {
  return convert(get_handle_kind(parse_handle(args)) == HandleKind::Argument);
}

static PyObject*
is_block(PyObject* self, PyObject* args) {
  return convert(get_handle_kind(parse_handle(args)) == HandleKind::BasicBlock);
}

static PyObject*
is_comdat(PyObject* self, PyObject* args) {
  return convert(get_handle_kind(parse_handle(args)) == HandleKind::Comdat);
}

static PyObject*
is_function(PyObject* self, PyObject* args) {
  return convert(get_handle_kind(parse_handle(args)) == HandleKind::Function);
}

static PyObject*
is_global(PyObject* self, PyObject* args) {
  return convert(get_handle_kind(parse_handle(args))
                 == HandleKind::GlobalVariable);
}

static PyObject*
is_instruction(PyObject* self, PyObject* args) {
  return convert(get_handle_kind(parse_handle(args))
                 == HandleKind::Instruction);
}

static PyObject*
is_metadata(PyObject* self, PyObject* args) {
  return convert(get_handle_kind(parse_handle(args)) == HandleKind::MDNode);
}

static PyObject*
is_module(PyObject* self, PyObject* args) {
  return convert(get_handle_kind(parse_handle(args)) == HandleKind::Module);
}

static PyObject*
is_struct(PyObject* self, PyObject* args) {
  return convert(get_handle_kind(parse_handle(args)) == HandleKind::StructType);
}

// Module interface

static PyObject*
module_create(PyObject* self, PyObject* args) {
  const char* file = "";
  if(!PyArg_ParseTuple(args, "s", &file))
    return nullptr;

  // Module::create returns a std::unique_ptr. We don't want the caller to
  // own this, so we just release it from the returned pointer and hand
  // the pointer off to the caller. It is the caller's responsibilty to
  // call lb_module_free() to release the Module
  return get_py_handle(lb::Module::create(file).release(), HandleKind::Module);
}

static PyObject*
module_get_code(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Module>(parse_handle(args))->get_code());
}

static PyObject*
module_get_aliases(PyObject* self, PyObject* args) {
  const lb::Module* module = get_object<lb::Module>(parse_handle(args));
  PyObject* aliases        = PyList_New(0);
  for(const lb::GlobalAlias* alias : module->aliases())
    PyList_Append(aliases, get_py_handle(alias, HandleKind::GlobalAlias));

  Py_INCREF(aliases);
  return aliases;
}

static PyObject*
module_get_comdats(PyObject* self, PyObject* args) {
  const lb::Module* module = get_object<lb::Module>(parse_handle(args));
  PyObject* comdats        = PyList_New(0);
  for(const lb::Comdat* comdat : module->comdats())
    PyList_Append(comdats, get_py_handle(comdat, HandleKind::Comdat));

  Py_INCREF(comdats);
  return comdats;
}

static PyObject*
module_get_functions(PyObject* self, PyObject* args) {
  const lb::Module* module = get_object<lb::Module>(parse_handle(args));
  PyObject* functions      = PyList_New(0);
  for(const lb::Function* f : module->functions())
    PyList_Append(functions, get_py_handle(f, HandleKind::Function));

  Py_INCREF(functions);
  return functions;
}

static PyObject*
module_get_globals(PyObject* self, PyObject* args) {
  const lb::Module* module = get_object<lb::Module>(parse_handle(args));
  PyObject* globals        = PyList_New(0);
  for(const lb::GlobalVariable* g : module->globals())
    PyList_Append(globals, get_py_handle(g, HandleKind::GlobalVariable));

  Py_INCREF(globals);
  return globals;
}

static PyObject*
module_get_structs(PyObject* self, PyObject* args) {

  const lb::Module* module = get_object<lb::Module>(parse_handle(args));
  PyObject* structs        = PyList_New(0);
  for(const lb::StructType* s : module->structs())
    PyList_Append(structs, get_py_handle(s, HandleKind::StructType));

  Py_INCREF(structs);
  return structs;
}

static PyObject*
module_free(PyObject* self, PyObject* args) {
  delete get_object<lb::Module>(parse_handle(args));

  Py_INCREF(Py_None);
  return Py_None;
}

// Alias interface

static PyObject*
alias_get_llvm_defn(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::GlobalAlias>(parse_handle(args))->get_llvm_defn());
}

static PyObject*
alias_get_llvm_span(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::GlobalAlias>(parse_handle(args))->get_llvm_span());
}

static PyObject*
alias_get_uses(PyObject* self, PyObject* args) {
  return convert(get_object<lb::GlobalAlias>(parse_handle(args))->uses());
}

static PyObject*
alias_get_indirect_uses(PyObject* self, PyObject* args) {
  lb::error() << "UNIMPLEMENTED: alias_get_indirect_uses()\n";
  return nullptr;
}

static PyObject*
alias_get_tag(PyObject* self, PyObject* args) {
  return convert(get_object<lb::GlobalAlias>(parse_handle(args))->get_tag());
}

static PyObject*
alias_get_llvm_name(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::GlobalAlias>(parse_handle(args))->get_llvm_name());
}

static PyObject*
alias_is_artificial(PyObject* self, PyObject* args) {
  return convert(true);
}

// Argument interface

static PyObject*
arg_get_llvm_defn(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Argument>(parse_handle(args))->get_llvm_defn());
}

static PyObject*
arg_get_llvm_span(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Argument>(parse_handle(args))->get_llvm_span());
}

static PyObject*
arg_get_source_defn(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::Argument>(parse_handle(args))->get_source_defn());
}

static PyObject*
arg_get_uses(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Argument>(parse_handle(args))->uses());
}

static PyObject*
arg_get_indirect_uses(PyObject* self, PyObject* args) {
  lb::error() << "UNIMPLEMENTED: argument_get_indirect_uses()\n";
  return nullptr;
}

static PyObject*
arg_get_tag(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Argument>(parse_handle(args))->get_tag());
}

static PyObject*
arg_get_llvm_name(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Argument>(parse_handle(args))->get_llvm_name());
}

static PyObject*
arg_get_source_name(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::Argument>(parse_handle(args))->get_source_name());
}

static PyObject*
arg_is_artificial(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Argument>(parse_handle(args))->is_artificial());
}

// Basic block interface

static PyObject*
block_get_llvm_defn(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::BasicBlock>(parse_handle(args))->get_llvm_defn());
}

static PyObject*
block_get_llvm_span(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::BasicBlock>(parse_handle(args))->get_llvm_span());
}

static PyObject*
block_get_uses(PyObject* self, PyObject* args) {
  return convert(get_object<lb::BasicBlock>(parse_handle(args))->uses());
}

static PyObject*
block_get_indirect_uses(PyObject* self, PyObject* args) {
  // There are no indiret uses of basic blocks, so just return the uses
  return block_get_uses(self, args);
}

static PyObject*
block_get_tag(PyObject* self, PyObject* args) {
  return convert(get_object<lb::BasicBlock>(parse_handle(args))->get_tag());
}

static PyObject*
block_is_artificial(PyObject* self, PyObject* args) {
  return convert(get_object<lb::BasicBlock>(parse_handle(args))
                     ->get_function()
                     .is_artificial());
}

static PyObject*
block_get_instructions(PyObject* self, PyObject* args) {
  const lb::BasicBlock* bb = get_object<lb::BasicBlock>(parse_handle(args));
  PyObject* insts          = PyList_New(0);
  for(const lb::Instruction* inst : bb->instructions())
    PyList_Append(insts, get_py_handle(inst, HandleKind::Instruction));

  Py_INCREF(insts);
  return insts;
}

// Comdat interface

static PyObject*
comdat_get_llvm_defn(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Comdat>(parse_handle(args))->get_llvm_defn());
}

static PyObject*
comdat_get_self_llvm_defn(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::Comdat>(parse_handle(args))->get_self_llvm_defn());
}

static PyObject*
comdat_get_llvm_span(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Comdat>(parse_handle(args))->get_llvm_span());
}

static PyObject*
comdat_get_tag(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Comdat>(parse_handle(args))->get_tag());
}

// Function interface

static PyObject*
func_get_llvm_defn(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Function>(parse_handle(args))->get_llvm_defn());
}

static PyObject*
func_get_llvm_span(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Function>(parse_handle(args))->get_llvm_span());
}

static PyObject*
func_get_source_defn(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::Function>(parse_handle(args))->get_source_defn());
}

static PyObject*
func_get_uses(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Function>(parse_handle(args))->uses());
}

static PyObject*
func_get_indirect_uses(PyObject* self, PyObject* args) {
  lb::error() << "UNIMPLEMENTED: function_get_indirect_uses()\n";
  return nullptr;
}

static PyObject*
func_get_tag(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Function>(parse_handle(args))->get_tag());
}

static PyObject*
func_get_llvm_name(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Function>(parse_handle(args))->get_llvm_name());
}

static PyObject*
func_get_source_name(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::Function>(parse_handle(args))->get_source_name());
}

static PyObject*
func_get_full_name(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Function>(parse_handle(args))->get_full_name());
}

static PyObject*
func_is_artificial(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Function>(parse_handle(args))->is_artificial());
}

static PyObject*
func_is_mangled(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Function>(parse_handle(args))->is_mangled());
}

static PyObject*
func_get_args(PyObject* self, PyObject* args) {
  const lb::Function* f = get_object<lb::Function>(parse_handle(args));
  PyObject* arguments   = PyList_New(0);
  for(const lb::Argument* arg : f->arguments())
    PyList_Append(arguments, get_py_handle(arg, HandleKind::Argument));

  Py_INCREF(arguments);
  return arguments;
}

static PyObject*
func_get_blocks(PyObject* self, PyObject* args) {
  const lb::Function* f = get_object<lb::Function>(parse_handle(args));
  PyObject* blocks      = PyList_New(0);
  for(const lb::BasicBlock* bb : f->blocks())
    PyList_Append(blocks, get_py_handle(bb, HandleKind::BasicBlock));

  Py_INCREF(blocks);
  return blocks;
}

static PyObject*
func_get_comdat(PyObject* self, PyObject* args) {
  const lb::Function* f = get_object<lb::Function>(parse_handle(args));
  if(const lb::Comdat* comdat = f->get_comdat())
    return get_py_handle(comdat, HandleKind::Comdat);
  return convert(HANDLE_NULL);
}

// Global interface

static PyObject*
global_get_llvm_defn(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::GlobalVariable>(parse_handle(args))->get_llvm_defn());
}

static PyObject*
global_get_llvm_span(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::GlobalVariable>(parse_handle(args))->get_llvm_span());
}

static PyObject*
global_get_source_defn(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::GlobalVariable>(parse_handle(args))->get_source_defn());
}

static PyObject*
global_get_uses(PyObject* self, PyObject* args) {
  return convert(get_object<lb::GlobalVariable>(parse_handle(args))->uses());
}

static PyObject*
global_get_indirect_uses(PyObject* self, PyObject* args) {
  lb::error() << "UNIMPLEMENTED: global_get_indirect_uses()\n";
  return nullptr;
}

static PyObject*
global_get_tag(PyObject* self, PyObject* args) {
  return convert(get_object<lb::GlobalVariable>(parse_handle(args))->get_tag());
}

static PyObject*
global_get_llvm_name(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::GlobalVariable>(parse_handle(args))->get_llvm_name());
}

static PyObject*
global_get_source_name(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::GlobalVariable>(parse_handle(args))->get_source_name());
}

static PyObject*
global_get_full_name(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::GlobalVariable>(parse_handle(args))->get_full_name());
}

static PyObject*
global_is_artificial(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::GlobalVariable>(parse_handle(args))->is_artificial());
}

static PyObject*
global_is_mangled(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::GlobalVariable>(parse_handle(args))->is_mangled());
}

static PyObject*
global_get_comdat(PyObject* self, PyObject* args) {
  const lb::GlobalVariable* g
      = get_object<lb::GlobalVariable>(parse_handle(args));
  if(const lb::Comdat* comdat = g->get_comdat())
    return get_py_handle(comdat, HandleKind::Comdat);
  return convert(HANDLE_NULL);
}

// Instruction interface

static PyObject*
inst_get_llvm_defn(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::Instruction>(parse_handle(args))->get_llvm_defn());
}

static PyObject*
inst_get_llvm_span(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::Instruction>(parse_handle(args))->get_llvm_span());
}

static PyObject*
inst_get_source_defn(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::Instruction>(parse_handle(args))->get_source_defn());
}

static PyObject*
inst_get_uses(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Instruction>(parse_handle(args))->uses());
}

static PyObject*
inst_get_indirect_uses(PyObject* self, PyObject* args) {
  lb::error() << "UNIMPLEMENTED: inst_get_indirect_uses()\n";
  return nullptr;
}

static PyObject*
inst_get_tag(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Instruction>(parse_handle(args))->get_tag());
}

static PyObject*
inst_is_artificial(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Instruction>(parse_handle(args))
                     ->get_function()
                     .is_artificial());
}

// Metadata interface

static PyObject*
md_get_llvm_defn(PyObject* self, PyObject* args) {
  return convert(get_object<lb::MDNode>(parse_handle(args))->get_llvm_defn());
}

static PyObject*
md_get_llvm_span(PyObject* self, PyObject* args) {
  return convert(get_object<lb::MDNode>(parse_handle(args))->get_llvm_span());
}

static PyObject*
md_get_uses(PyObject* self, PyObject* args) {
  return convert(get_object<lb::MDNode>(parse_handle(args))->uses());
}

static PyObject*
md_get_indirect_uses(PyObject* self, PyObject* args) {
  return md_get_uses(self, args);
}

static PyObject*
md_get_tag(PyObject* self, PyObject* args) {
  return convert(get_object<lb::MDNode>(parse_handle(args))->get_tag());
}

static PyObject*
md_is_artificial(PyObject* self, PyObject* args) {
  return convert(get_object<lb::MDNode>(parse_handle(args))->is_artificial());
}

// Struct interface

static PyObject*
struct_get_llvm_defn(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::StructType>(parse_handle(args))->get_llvm_defn());
}

static PyObject*
struct_get_llvm_span(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::StructType>(parse_handle(args))->get_llvm_span());
}

static PyObject*
struct_get_source_defn(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::StructType>(parse_handle(args))->get_source_defn());
}

static PyObject*
struct_get_uses(PyObject* self, PyObject* args) {
  return convert(get_object<lb::StructType>(parse_handle(args))->uses());
}

static PyObject*
struct_get_indirect_uses(PyObject* self, PyObject* args) {
  // Indirect uses don't make much sense for a struct type, so just return
  // the uses
  return struct_get_uses(self, args);
}

static PyObject*
struct_get_tag(PyObject* self, PyObject* args) {
  return convert(get_object<lb::StructType>(parse_handle(args))->get_tag());
}

static PyObject*
struct_get_llvm_name(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::StructType>(parse_handle(args))->get_llvm_name());
}

static PyObject*
struct_get_source_name(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::StructType>(parse_handle(args))->get_source_name());
}

static PyObject*
struct_get_full_name(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::StructType>(parse_handle(args))->get_full_name());
}

static PyObject*
struct_is_artificial(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::StructType>(parse_handle(args))->is_artificial());
}

// Generic interface

static PyObject*
entity_get_llvm_defn(PyObject* self, PyObject* args) {
  Handle handle = parse_handle(args);
  switch(get_handle_kind(handle)) {
  case HandleKind::Argument:
    return arg_get_llvm_defn(self, args);
  case HandleKind::BasicBlock:
    return block_get_llvm_defn(self, args);
  case HandleKind::Function:
    return func_get_llvm_defn(self, args);
  case HandleKind::GlobalAlias:
    return alias_get_llvm_defn(self, args);
  case HandleKind::GlobalVariable:
    return global_get_llvm_defn(self, args);
  case HandleKind::Instruction:
    return inst_get_llvm_defn(self, args);
  case HandleKind::MDNode:
    return md_get_llvm_defn(self, args);
  case HandleKind::StructType:
    return struct_get_llvm_defn(self, args);
  default:
    lb::warning() << "Cannot get LLVM definition for entity: "
                  << get_handle_kind_name(handle) << "\n";
    break;
  }

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject*
entity_get_llvm_span(PyObject* self, PyObject* args) {
  Handle handle = parse_handle(args);
  switch(get_handle_kind(handle)) {
  case HandleKind::Argument:
    return arg_get_llvm_span(self, args);
  case HandleKind::BasicBlock:
    return block_get_llvm_span(self, args);
  case HandleKind::Function:
    return func_get_llvm_span(self, args);
  case HandleKind::GlobalAlias:
    return alias_get_llvm_span(self, args);
  case HandleKind::GlobalVariable:
    return global_get_llvm_span(self, args);
  case HandleKind::Instruction:
    return inst_get_llvm_span(self, args);
  case HandleKind::MDNode:
    return md_get_llvm_span(self, args);
  case HandleKind::StructType:
    return struct_get_llvm_span(self, args);
  default:
    lb::warning() << "Cannot get LLVM definition for entity: "
                  << get_handle_kind_name(handle) << "\n";
    break;
  }

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject*
entity_get_source_defn(PyObject* self, PyObject* args) {
  Handle handle = parse_handle(args);
  switch(get_handle_kind(handle)) {
  case HandleKind::Argument:
    return arg_get_source_defn(self, args);
  case HandleKind::Function:
    return func_get_source_defn(self, args);
  case HandleKind::GlobalVariable:
    return global_get_source_defn(self, args);
  case HandleKind::Instruction:
    return inst_get_source_defn(self, args);
  case HandleKind::StructType:
    return struct_get_llvm_defn(self, args);
  default:
    lb::warning() << "Cannot get LLVM definition for entity: "
                  << get_handle_kind_name(handle) << "\n";
    break;
  }

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject*
entity_get_uses(PyObject* self, PyObject* args) {
  Handle handle = parse_handle(args);
  switch(get_handle_kind(handle)) {
  case HandleKind::Argument:
    return arg_get_uses(self, args);
  case HandleKind::BasicBlock:
    return block_get_uses(self, args);
  case HandleKind::Function:
    return func_get_uses(self, args);
  case HandleKind::GlobalAlias:
    return alias_get_uses(self, args);
  case HandleKind::GlobalVariable:
    return global_get_uses(self, args);
  case HandleKind::Instruction:
    return global_get_uses(self, args);
  case HandleKind::MDNode:
    return md_get_uses(self, args);
  case HandleKind::StructType:
    return struct_get_uses(self, args);
  default:
    lb::error() << "Cannot get uses for entity: "
                << get_handle_kind_name(handle) << "\n";
    return nullptr;
  }

  return PyList_New(0);
}

static PyObject*
entity_get_indirect_uses(PyObject* self, PyObject* args) {
  Handle handle = parse_handle(args);
  switch(get_handle_kind(handle)) {
  case HandleKind::Argument:
    return arg_get_indirect_uses(self, args);
  case HandleKind::BasicBlock:
    return block_get_indirect_uses(self, args);
  case HandleKind::Function:
    return func_get_indirect_uses(self, args);
  case HandleKind::GlobalAlias:
    return alias_get_indirect_uses(self, args);
  case HandleKind::GlobalVariable:
    return global_get_indirect_uses(self, args);
  case HandleKind::Instruction:
    return global_get_indirect_uses(self, args);
  case HandleKind::MDNode:
    return md_get_indirect_uses(self, args);
  case HandleKind::StructType:
    return struct_get_indirect_uses(self, args);
  default:
    lb::error() << "Cannot get uses for entity: "
                << get_handle_kind_name(handle) << "\n";
    return nullptr;
  }

  return PyList_New(0);
}

static PyObject*
entity_get_tag(PyObject* self, PyObject* args) {
  Handle handle = parse_handle(args);
  switch(get_handle_kind(handle)) {
  case HandleKind::Argument:
    return arg_get_tag(self, args);
  case HandleKind::BasicBlock:
    return block_get_tag(self, args);
  case HandleKind::Function:
    return func_get_tag(self, args);
  case HandleKind::GlobalAlias:
    return alias_get_tag(self, args);
  case HandleKind::GlobalVariable:
    return global_get_tag(self, args);
  case HandleKind::Instruction:
    return inst_get_tag(self, args);
  case HandleKind::MDNode:
    return md_get_tag(self, args);
  case HandleKind::StructType:
    return struct_get_tag(self, args);
  default:
    lb::error() << "Cannot get tag for entity: " << get_handle_kind_name(handle)
                << "\n";
    return nullptr;
  }

  return convert(llvm::StringRef());
}

static PyObject*
entity_get_llvm_name(PyObject* self, PyObject* args) {
  Handle handle = parse_handle(args);
  switch(get_handle_kind(handle)) {
  case HandleKind::Argument:
    return arg_get_llvm_name(self, args);
  case HandleKind::Function:
    return func_get_llvm_name(self, args);
  case HandleKind::GlobalAlias:
    return alias_get_llvm_name(self, args);
  case HandleKind::GlobalVariable:
    return global_get_llvm_name(self, args);
  case HandleKind::StructType:
    return struct_get_llvm_name(self, args);
  default:
    lb::warning() << "Cannot get LLVM name for entity: "
                  << get_handle_kind_name(handle) << "\n";
    break;
  }

  return convert(llvm::StringRef());
}

static PyObject*
entity_get_source_name(PyObject* self, PyObject* args) {
  Handle handle = parse_handle(args);
  switch(get_handle_kind(handle)) {
  case HandleKind::Argument:
    return arg_get_source_name(self, args);
  case HandleKind::Function:
    return func_get_source_name(self, args);
  case HandleKind::GlobalVariable:
    return global_get_source_name(self, args);
  case HandleKind::StructType:
    return struct_get_source_name(self, args);
  default:
    lb::warning() << "Cannot get source name for entity: "
                  << get_handle_kind_name(handle) << "\n";
    break;
  }

  return convert(llvm::StringRef());
}

static PyObject*
entity_get_full_name(PyObject* self, PyObject* args) {
  Handle handle = parse_handle(args);
  switch(get_handle_kind(handle)) {
  case HandleKind::Function:
    return func_get_full_name(self, args);
  case HandleKind::GlobalVariable:
    return global_get_full_name(self, args);
  case HandleKind::StructType:
    return struct_get_full_name(self, args);
  default:
    lb::warning() << "Cannot get full name for entity: "
                  << get_handle_kind_name(handle) << "\n";
    break;
  }

  return convert(llvm::StringRef());
}

static PyObject*
entity_is_artificial(PyObject* self, PyObject* args) {
  Handle handle = parse_handle(args);
  switch(get_handle_kind(handle)) {
  case HandleKind::Argument:
    return arg_is_artificial(self, args);
  case HandleKind::BasicBlock:
    return block_is_artificial(self, args);
  case HandleKind::Function:
    return func_is_artificial(self, args);
  case HandleKind::GlobalAlias:
    return alias_is_artificial(self, args);
  case HandleKind::GlobalVariable:
    return global_is_artificial(self, args);
  case HandleKind::Instruction:
    return inst_is_artificial(self, args);
  case HandleKind::MDNode:
    return md_is_artificial(self, args);
  case HandleKind::StructType:
    return struct_is_artificial(self, args);
  default:
    lb::error() << "Cannot check if entity is artificial: "
                << get_handle_kind_name(handle) << "\n";
    return nullptr;
  }
}

#define FUNC(name, descr)                                                      \
  { #name, (PyCFunction)name, METH_VARARGS, descr }

static PyMethodDef module_methods[] = {
    // Utils
    FUNC(is_alias, "True if the handle is a GlobalAlias"),
    FUNC(is_argument, "True if the handle is an Argument"),
    FUNC(is_block, "True if the handle is a BasicBlock"),
    FUNC(is_comdat, "True if the handle is a Comdat"),
    FUNC(is_function, "True if the handle is a Function"),
    FUNC(is_global, "True if the handle is a GlobalVariable"),
    FUNC(is_instruction, "True if the handle is an Instruction"),
    FUNC(is_metadata, "True if the handle is an MDNode"),
    FUNC(is_module, "True if the handle is a Module"),
    FUNC(is_struct, "True if the handle is a struct"),
    FUNC(get_null_handle, "Returns a handle representing None"),

    // Module interface
    FUNC(module_create, "Create a new module and return a handle to it"),
    FUNC(module_free, "Free a module created by module_create"),
    FUNC(module_get_code, "LLVM-IR for the module"),
    FUNC(module_get_aliases, "A list of handles to the aliases in the module"),
    FUNC(module_get_comdats, "A list of handles to the comdats in the module"),
    FUNC(module_get_functions,
         "A list of handles to the functions in the module"),
    FUNC(module_get_globals, "A list of handles to the globals in the module"),
    FUNC(module_get_structs, "A list of handles to the structs in the module"),

    // Alias interface
    FUNC(alias_get_llvm_defn, "LLVM definition range of the alias"),
    FUNC(alias_get_llvm_span, "LLVM span of the alias"),
    FUNC(alias_get_uses, "Uses of the alias"),
    FUNC(alias_get_indirect_uses, "Indirect uses of the alias"),
    FUNC(alias_get_tag, "Tag of the alias"),
    FUNC(alias_get_llvm_name, "LLVM name of the alias"),
    FUNC(alias_is_artificial, "Always returns true"),

    // Argument interface
    FUNC(arg_get_llvm_defn, "LLVM definition range of the argument"),
    FUNC(arg_get_llvm_span, "LLVM span of the argument"),
    FUNC(arg_get_source_defn, "Source location of the argument"),
    FUNC(arg_get_uses, "Uses of the argument"),
    FUNC(arg_get_indirect_uses, "Indirect uses of the argument"),
    FUNC(arg_get_tag, "Tag of the argument"),
    FUNC(arg_get_llvm_name, "LLVM name of the argument"),
    FUNC(arg_get_source_name, "Source name of the argument"),
    FUNC(arg_is_artificial,
         "True if the argument was inserted by the compiler"),

    // Basic block interface
    FUNC(block_get_llvm_defn, "LLVM definition range of the basic block"),
    FUNC(block_get_llvm_span, "LLVM span of the basic block"),
    FUNC(block_get_uses, "Uses of the block"),
    FUNC(block_get_indirect_uses, "Indirect uses of the block"),
    FUNC(block_get_tag, "Tag of the basic block"),
    FUNC(block_is_artificial, "True if the parent function is artificial"),
    FUNC(block_get_instructions,
         "List of handls to the instructions in the basic block"),

    // Comdat interface
    FUNC(comdat_get_llvm_defn, "LLVM definition range of the comdat"),
    FUNC(comdat_get_self_llvm_defn, "LLVM definition of the comdat itself"),
    FUNC(comdat_get_llvm_span, "LLVM span of the comdat"),
    FUNC(comdat_get_tag, "Tag of the comdat"),

    // Function interface
    FUNC(func_get_llvm_defn, "LLVM definition range of the function"),
    FUNC(func_get_llvm_span, "LLVM span of the function"),
    FUNC(func_get_source_defn, "Source location of the function"),
    FUNC(func_get_uses, "Uses of the function"),
    FUNC(func_get_indirect_uses, "Indirect uses of the function"),
    FUNC(func_get_tag, "Tag of the function"),
    FUNC(func_get_llvm_name, "LLVM name of the function"),
    FUNC(func_get_source_name, "Source name of the function"),
    FUNC(func_get_full_name, "Full name of the function"),
    FUNC(func_is_artificial,
         "True if the function was generated by the compiler"),
    FUNC(func_is_mangled, "True if the LLVM name of the function is mangled"),
    FUNC(func_get_args, "List of handles to the function arguments"),
    FUNC(func_get_blocks,
         "List of handles to the basic blocks in the function"),
    FUNC(func_get_comdat,
         "Handle to the comdat of the function or HANDLE_NULL if the function "
         "does not have a comdat"),

    // Global interface
    FUNC(global_get_llvm_defn, "LLVM definition range of the global"),
    FUNC(global_get_llvm_span, "LLVM span of the global"),
    FUNC(global_get_source_defn, "Source location of the global"),
    FUNC(global_get_uses, "Uses of the global"),
    FUNC(global_get_indirect_uses, "Indirect uses of the global"),
    FUNC(global_get_tag, "Tag of the global"),
    FUNC(global_get_llvm_name, "LLVM name of the global"),
    FUNC(global_get_source_name, "Source name of the global"),
    FUNC(global_is_artificial,
         "True if the global was generated by the compiler"),
    FUNC(global_is_mangled, "True if the LLVM name of the global is mangled"),
    FUNC(global_get_comdat,
         "Handle to the comdat of the global variable or HANDLE_NULL if the "
         "global does not have a comdat"),

    // Instruction interface
    FUNC(inst_get_llvm_defn, "LLVM definition range of the instruction"),
    FUNC(inst_get_llvm_span, "LLVM span of the instruction"),
    FUNC(inst_get_source_defn, "Source location of the instruction"),
    FUNC(inst_get_uses, "Uses of the instruction"),
    FUNC(inst_get_indirect_uses, "Indirect uses of the instruction"),
    FUNC(inst_get_tag, "Tag of the instruction (may be a LLVM slot)"),
    FUNC(inst_is_artificial, "True if the parent function is artificial"),

    // Metadata interface
    FUNC(md_get_llvm_defn, "LLVM definition range of the metadata node"),
    FUNC(md_get_llvm_span, "LLVM span of the metadata node"),
    FUNC(md_get_uses, "Uses of the metadata node"),
    FUNC(md_get_indirect_uses, "Indirect uses of the metadata node"),
    FUNC(md_get_tag, "Tag of the metadata node"),
    FUNC(md_is_artificial, "Always returns true"),

    // Struct interface
    FUNC(struct_get_llvm_defn, "LLVM definition range of the struct"),
    FUNC(struct_get_llvm_span, "LLVM span of the struct"),
    FUNC(struct_get_source_defn, "Source location of the struct"),
    FUNC(struct_get_uses, "Uses of the struct"),
    FUNC(struct_get_indirect_uses, "Indirect uses of the struct"),
    FUNC(struct_get_tag, "Tag of the struct"),
    FUNC(struct_get_llvm_name, "LLVM name of the struct"),
    FUNC(struct_get_source_name, "Source name of the struct"),
    FUNC(struct_get_full_name, "Full name of the struct"),
    FUNC(struct_is_artificial,
         "True if the struct was generated by the compiler"),

    // Generic interface
    FUNC(entity_get_llvm_defn, "LLVM definition range of the entity"),
    FUNC(entity_get_llvm_span, "LLVM span of the entity"),
    FUNC(entity_get_source_defn, "Source location of the entity"),
    FUNC(entity_get_uses, "Uses of the entity"),
    FUNC(entity_get_indirect_uses, "Indirect uses of the entity"),
    FUNC(entity_get_tag, "Tag of the entity"),
    FUNC(entity_get_llvm_name, "LLVM name of the entity"),
    FUNC(entity_get_source_name, "Source name of the entity"),
    FUNC(entity_get_full_name, "Full name of the entity"),
    FUNC(entity_is_artificial,
         "True if the entity was generated by the compiler"),

    // End sentinel
    {nullptr, nullptr, 0, nullptr},
};

static struct PyModuleDef module_def = {
    PyModuleDef_HEAD_INIT,
    "llvm_browse",
    "Python module to interface with for libLLVMBrowse",
    -1,
    module_methods,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
};

PyMODINIT_FUNC
PyInit_llvm_browse(void) {
  // Initialize types
  PySourcePoint = PyStructSequence_NewType(&PySourcePointDesc);
  PySourceRange = PyStructSequence_NewType(&PySourceRangeDesc);
  PyLLVMRange   = PyStructSequence_NewType(&PyLLVMRangeDesc);

  PyObject* module = PyModule_Create(&module_def);
  if(not module)
    lb::error() << "Error creating module";

  return module;
}
#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "lib/Argument.h"
#include "lib/BasicBlock.h"
#include "lib/Comdat.h"
#include "lib/Definition.h"
#include "lib/Function.h"
#include "lib/GlobalAlias.h"
#include "lib/GlobalVariable.h"
#include "lib/Instruction.h"
#include "lib/Logging.h"
#include "lib/MDNode.h"
#include "lib/Module.h"
#include "lib/StructType.h"
#include "lib/Use.h"

#include <llvm/ADT/iterator_range.h>
#include <llvm/Support/Casting.h>

#include <cstdio>
#include <type_traits>

using llvm::cast;
using llvm::dyn_cast;
using llvm::isa;

// Typedefs

// The tags are added to the Handle to be able to determine the dynamic type
// of the object from the handle
enum class HandleKind {
  Invalid        = 0x0,
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
  Use            = 0xB,
  Definition     = 0xC,
  Mask           = 0xf,
};

using Handle = uint64_t;

// Invalid handle
static const int HANDLE_NULL = 0;

template<typename T, std::enable_if_t<!std::is_pointer<T>::value, int> = 0>
static Handle
get_handle(const T& ptr, HandleKind tag) {
  return reinterpret_cast<Handle>(&ptr) | static_cast<Handle>(tag);
}

template<typename T, std::enable_if_t<!std::is_pointer<T>::value, int> = 0>
static PyObject*
get_py_handle(const T& ptr, HandleKind tag) {
  return PyLong_FromLong(get_handle(ptr, tag));
}

static PyObject*
get_py_handle(const lb::Value& v) {
  if(const auto* alias = dyn_cast<lb::GlobalAlias>(&v))
    return get_py_handle(*alias, HandleKind::GlobalAlias);
  else if(const auto* arg = dyn_cast<lb::Argument>(&v))
    return get_py_handle(*arg, HandleKind::Argument);
  else if(const auto* bb = dyn_cast<lb::BasicBlock>(&v))
    return get_py_handle(*bb, HandleKind::BasicBlock);
  else if(const auto* f = dyn_cast<lb::Function>(&v))
    return get_py_handle(*f, HandleKind::Function);
  else if(const auto* g = dyn_cast<lb::GlobalVariable>(&v))
    return get_py_handle(*g, HandleKind::GlobalVariable);
  else if(const auto* inst = dyn_cast<lb::Instruction>(&v))
    return get_py_handle(*inst, HandleKind::Instruction);
  else
    lb::error() << "Cannot get handle for lb::Value: "
                << static_cast<int>(v.get_kind()) << "\n";

  return nullptr;
}

static PyObject*
get_py_handle(const lb::INavigable& v) {
  if(const auto* alias = dyn_cast<lb::GlobalAlias>(&v))
    return get_py_handle(*alias, HandleKind::GlobalAlias);
  else if(const auto* arg = dyn_cast<lb::Argument>(&v))
    return get_py_handle(*arg, HandleKind::Argument);
  else if(const auto* bb = dyn_cast<lb::BasicBlock>(&v))
    return get_py_handle(*bb, HandleKind::BasicBlock);
  else if(const auto* comdat = dyn_cast<lb::Comdat>(&v))
    return get_py_handle(*comdat, HandleKind::Comdat);
  else if(const auto* f = dyn_cast<lb::Function>(&v))
    return get_py_handle(*f, HandleKind::Function);
  else if(const auto* g = dyn_cast<lb::GlobalVariable>(&v))
    return get_py_handle(*g, HandleKind::GlobalVariable);
  else if(const auto* inst = dyn_cast<lb::Instruction>(&v))
    return get_py_handle(*inst, HandleKind::Instruction);
  else if(const auto* md = dyn_cast<lb::MDNode>(&v))
    return get_py_handle(*md, HandleKind::MDNode);
  else if(const auto* s = dyn_cast<lb::StructType>(&v))
    return get_py_handle(*s, HandleKind::StructType);
  else
    lb::error() << "Cannot get handle for lb::Navigable: "
                << static_cast<int>(v.get_kind()) << "\n";

  return nullptr;
}

static PyObject*
get_py_handle() {
  return PyLong_FromLong(HANDLE_NULL);
}

template<typename T>
static const T&
get_object(Handle handle) {
  return *reinterpret_cast<T*>(handle & ~static_cast<Handle>(HandleKind::Mask));
}

static HandleKind
get_handle_kind(Handle handle) {
  if(handle == HANDLE_NULL)
    return HandleKind::Invalid;
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
  case HandleKind::Use:
    return "Use";
  case HandleKind::Definition:
    return "Definition";
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
convert(unsigned n) {
  return PyLong_FromLong(n);
}

static PyObject*
convert(lb::Offset offset) {
  return PyLong_FromLong(offset);
}

static PyObject*
convert(llvm::StringRef s) {
  if(s.size())
    return PyUnicode_FromString(s.data());
  return PyUnicode_FromString("");
}

static PyObject*
convert(const std::string& s) {
  if(s.size())
    return PyUnicode_FromString(s.c_str());
  return PyUnicode_FromString("");
}

static PyObject*
convert(const lb::SourceRange& range) {
  PyObject* py = Py_None;
  if(range) {
    PyObject* py_begin = PyStructSequence_New(PySourcePoint);
    PyStructSequence_SetItem(
        py_begin, 0, PyLong_FromLong(range.get_begin_line()));
    PyStructSequence_SetItem(
        py_begin, 1, PyLong_FromLong(range.get_begin_column()));
    Py_INCREF(py_begin);

    PyObject* py_end = PyStructSequence_New(PySourcePoint);
    PyStructSequence_SetItem(py_end, 0, PyLong_FromLong(range.get_end_line()));
    PyStructSequence_SetItem(
        py_end, 1, PyLong_FromLong(range.get_end_column()));
    Py_INCREF(py_end);

    py = PyStructSequence_New(PySourceRange);
    if(range.get_file())
      PyStructSequence_SetItem(py, 0, PyUnicode_FromString(range.get_file()));
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
    PyStructSequence_SetItem(py, 0, PyLong_FromLong(range.get_begin()));
    PyStructSequence_SetItem(py, 1, PyLong_FromLong(range.get_end()));
  }

  Py_INCREF(py);
  return py;
}

static PyObject*
convert(llvm::iterator_range<lb::INavigable::Iterator> uses) {
  PyObject* list = PyList_New(0);
  for(const lb::Use* use : uses)
    PyList_Append(list, get_py_handle(*use, HandleKind::Use));

  Py_INCREF(list);
  return list;
}

// PUBLIC methods

// Utils

static PyObject*
get_null_handle(PyObject* self, PyObject* args) {
  return get_py_handle();
}

static PyObject*
is_null_handle(PyObject* self, PyObject* args) {
  return convert(parse_handle(args) == HANDLE_NULL);
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

static PyObject*
is_use(PyObject* self, PyObject* args) {
  return convert(get_handle_kind(parse_handle(args)) == HandleKind::Use);
}

static PyObject*
is_def(PyObject* self, PyObject* args) {
  return convert(get_handle_kind(parse_handle(args)) == HandleKind::Definition);
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
  return get_py_handle(*lb::Module::create(file).release(), HandleKind::Module);
}

static PyObject*
module_get_code(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Module>(parse_handle(args)).get_code());
}

static PyObject*
module_get_aliases(PyObject* self, PyObject* args) {
  const auto& module = get_object<lb::Module>(parse_handle(args));
  PyObject* aliases        = PyList_New(0);
  for(const lb::GlobalAlias& alias : module.aliases())
    PyList_Append(aliases, get_py_handle(alias, HandleKind::GlobalAlias));

  Py_INCREF(aliases);
  return aliases;
}

static PyObject*
module_get_comdats(PyObject* self, PyObject* args) {
  const auto& module = get_object<lb::Module>(parse_handle(args));
  PyObject* comdats        = PyList_New(0);
  for(const lb::Comdat& comdat : module.comdats())
    PyList_Append(comdats, get_py_handle(comdat, HandleKind::Comdat));

  Py_INCREF(comdats);
  return comdats;
}

static PyObject*
module_get_functions(PyObject* self, PyObject* args) {
  const auto& module = get_object<lb::Module>(parse_handle(args));
  PyObject* functions      = PyList_New(0);
  for(const lb::Function& f : module.functions())
    PyList_Append(functions, get_py_handle(f, HandleKind::Function));

  Py_INCREF(functions);
  return functions;
}

static PyObject*
module_get_globals(PyObject* self, PyObject* args) {
  const auto& module = get_object<lb::Module>(parse_handle(args));
  PyObject* globals        = PyList_New(0);
  for(const lb::GlobalVariable& g : module.globals())
    PyList_Append(globals, get_py_handle(g, HandleKind::GlobalVariable));

  Py_INCREF(globals);
  return globals;
}

static PyObject*
module_get_structs(PyObject* self, PyObject* args) {

  const auto& module = get_object<lb::Module>(parse_handle(args));
  PyObject* structs        = PyList_New(0);
  for(const lb::StructType& s : module.structs())
    PyList_Append(structs, get_py_handle(s, HandleKind::StructType));

  Py_INCREF(structs);
  return structs;
}

static PyObject*
module_free(PyObject* self, PyObject* args) {
  delete &get_object<lb::Module>(parse_handle(args));

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject*
module_get_def_at(PyObject* self, PyObject* args) {
  Handle handle     = HANDLE_NULL;
  lb::Offset offset = 0;
  if(!PyArg_ParseTuple(args, "kk", &handle, &offset))
    return nullptr;

  const auto& module = get_object<lb::Module>(handle);
  if(const lb::Definition* def = module.get_definition_at(offset))
    return get_py_handle(*def, HandleKind::Definition);
  return get_py_handle();
}

static PyObject*
module_get_use_at(PyObject* self, PyObject* args) {
  Handle handle     = HANDLE_NULL;
  lb::Offset offset = 0;
  if(!PyArg_ParseTuple(args, "kk", &handle, &offset))
    return nullptr;

  const auto& module = get_object<lb::Module>(handle);
  if(const lb::Use* use = module.get_use_at(offset))
    return get_py_handle(*use, HandleKind::Use);
  return get_py_handle();
}

static PyObject*
module_get_comdat_at(PyObject* self, PyObject* args) {
  Handle handle     = HANDLE_NULL;
  lb::Offset offset = 0;
  if(!PyArg_ParseTuple(args, "kk", &handle, &offset))
    return nullptr;

  const auto& module = get_object<lb::Module>(handle);
  if(const lb::Comdat* comdat = module.get_comdat_at(offset))
    return get_py_handle(*comdat, HandleKind::Comdat);
  return get_py_handle();
}

static PyObject*
module_get_function_at(PyObject* self, PyObject* args) {
  Handle handle     = HANDLE_NULL;
  lb::Offset offset = 0;
  if(!PyArg_ParseTuple(args, "kk", &handle, &offset))
    return nullptr;

  const auto& module = get_object<lb::Module>(handle);
  if(const lb::Function* f = module.get_function_at(offset))
    return get_py_handle(*f, HandleKind::Function);
  return get_py_handle();
}

static PyObject*
module_get_block_at(PyObject* self, PyObject* args) {
  Handle handle     = HANDLE_NULL;
  lb::Offset offset = 0;
  if(!PyArg_ParseTuple(args, "kk", &handle, &offset))
    return nullptr;

  const auto& module = get_object<lb::Module>(handle);
  if(const lb::BasicBlock* bb = module.get_block_at(offset))
    return get_py_handle(*bb, HandleKind::BasicBlock);
  return get_py_handle();
}

static PyObject*
module_get_instruction_at(PyObject* self, PyObject* args) {
  Handle handle     = HANDLE_NULL;
  lb::Offset offset = 0;
  if(!PyArg_ParseTuple(args, "kk", &handle, &offset))
    return nullptr;

  const auto& module = get_object<lb::Module>(handle);
  if(const lb::Instruction* inst = module.get_instruction_at(offset))
    return get_py_handle(*inst, HandleKind::Instruction);
  return get_py_handle();
}

// Alias interface

static PyObject*
alias_has_llvm_defn(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::GlobalAlias>(parse_handle(args)).has_llvm_defn());
}

static PyObject*
alias_get_llvm_defn(PyObject* self, PyObject* args) {
  const auto& alias = get_object<lb::GlobalAlias>(parse_handle(args));
  if(alias.has_llvm_defn())
    return get_py_handle(alias.get_llvm_defn(), HandleKind::Definition);
  return get_py_handle();
}

static PyObject*
alias_get_llvm_span(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::GlobalAlias>(parse_handle(args)).get_llvm_span());
}

static PyObject*
alias_get_num_uses(PyObject* self, PyObject* args) {
  return convert(get_object<lb::GlobalAlias>(parse_handle(args)).uses());
}

static PyObject*
alias_get_uses(PyObject* self, PyObject* args) {
  return convert(get_object<lb::GlobalAlias>(parse_handle(args)).uses());
}

static PyObject*
alias_get_indirect_uses(PyObject* self, PyObject* args) {
  lb::error() << "UNIMPLEMENTED: alias_get_indirect_uses()\n";
  return nullptr;
}

static PyObject*
alias_get_tag(PyObject* self, PyObject* args) {
  return convert(get_object<lb::GlobalAlias>(parse_handle(args)).get_tag());
}

static PyObject*
alias_get_llvm_name(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::GlobalAlias>(parse_handle(args)).get_llvm_name());
}

static PyObject*
alias_has_source_info(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::GlobalAlias>(parse_handle(args)).has_source_info());
}

static PyObject*
alias_is_artificial(PyObject* self, PyObject* args) {
  return convert(true);
}

// Argument interface

static PyObject*
arg_has_llvm_defn(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Argument>(parse_handle(args)).has_llvm_defn());
}

static PyObject*
arg_get_llvm_defn(PyObject* self, PyObject* args) {
  const auto& arg = get_object<lb::Argument>(parse_handle(args));
  if(arg.has_llvm_defn())
    return get_py_handle(arg.get_llvm_defn(), HandleKind::Definition);
  return get_py_handle();
}

static PyObject*
arg_get_llvm_span(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Argument>(parse_handle(args)).get_llvm_span());
}

static PyObject*
arg_has_source_defn(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::Argument>(parse_handle(args)).has_source_defn());
}

static PyObject*
arg_get_source_defn(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::Argument>(parse_handle(args)).get_source_defn());
}

static PyObject*
arg_get_num_uses(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Argument>(parse_handle(args)).get_num_uses());
}

static PyObject*
arg_get_uses(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Argument>(parse_handle(args)).uses());
}

static PyObject*
arg_get_indirect_uses(PyObject* self, PyObject* args) {
  lb::error() << "UNIMPLEMENTED: argument_get_indirect_uses()\n";
  return nullptr;
}

static PyObject*
arg_get_tag(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Argument>(parse_handle(args)).get_tag());
}

static PyObject*
arg_get_llvm_name(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Argument>(parse_handle(args)).get_llvm_name());
}

static PyObject*
arg_get_source_name(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::Argument>(parse_handle(args)).get_source_name());
}

static PyObject*
arg_has_source_info(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::Argument>(parse_handle(args)).has_source_info());
}

static PyObject*
arg_is_artificial(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Argument>(parse_handle(args)).is_artificial());
}

// Basic block interface

static PyObject*
block_has_llvm_defn(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::BasicBlock>(parse_handle(args)).has_llvm_defn());
}

static PyObject*
block_get_llvm_defn(PyObject* self, PyObject* args) {
  const auto& bb = get_object<lb::BasicBlock>(parse_handle(args));
  if(bb.has_llvm_defn())
    return get_py_handle(bb.get_llvm_defn(), HandleKind::Definition);
  return get_py_handle();
}

static PyObject*
block_get_llvm_span(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::BasicBlock>(parse_handle(args)).get_llvm_span());
}

static PyObject*
block_has_source_defn(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::BasicBlock>(parse_handle(args)).has_source_defn());
}

static PyObject*
block_get_source_defn(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::BasicBlock>(parse_handle(args)).get_source_defn());
}

static PyObject*
block_get_num_uses(PyObject* self, PyObject* args) {
  return convert(get_object<lb::BasicBlock>(parse_handle(args)).get_num_uses());
}

static PyObject*
block_get_uses(PyObject* self, PyObject* args) {
  return convert(get_object<lb::BasicBlock>(parse_handle(args)).uses());
}

static PyObject*
block_get_indirect_uses(PyObject* self, PyObject* args) {
  // There are no indiret uses of basic blocks, so just return the uses
  return block_get_uses(self, args);
}

static PyObject*
block_get_tag(PyObject* self, PyObject* args) {
  return convert(get_object<lb::BasicBlock>(parse_handle(args)).get_tag());
}

static PyObject*
block_has_source_info(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::BasicBlock>(parse_handle(args)).has_source_info());
}

static PyObject*
block_is_artificial(PyObject* self, PyObject* args) {
  return convert(get_object<lb::BasicBlock>(parse_handle(args))
                     .get_function()
                     .is_artificial());
}

static PyObject*
block_get_instructions(PyObject* self, PyObject* args) {
  const auto& bb = get_object<lb::BasicBlock>(parse_handle(args));
  PyObject* insts          = PyList_New(0);
  for(const lb::Instruction& inst : bb.instructions())
    PyList_Append(insts, get_py_handle(inst, HandleKind::Instruction));

  Py_INCREF(insts);
  return insts;
}

// Comdat interface

static PyObject*
comdat_has_llvm_defn(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Comdat>(parse_handle(args)).has_llvm_defn());
}

static PyObject*
comdat_get_llvm_defn(PyObject* self, PyObject* args) {
  const auto& comdat = get_object<lb::Comdat>(parse_handle(args));
  if(comdat.has_llvm_defn())
    return get_py_handle(comdat.get_llvm_defn(), HandleKind::Definition);
  return get_py_handle();
}

static PyObject*
comdat_get_self_llvm_defn(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::Comdat>(parse_handle(args)).get_self_llvm_defn());
}

static PyObject*
comdat_get_llvm_span(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Comdat>(parse_handle(args)).get_llvm_span());
}

static PyObject*
comdat_get_tag(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Comdat>(parse_handle(args)).get_tag());
}

static PyObject*
comdat_get_llvm_name(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Comdat>(parse_handle(args)).get_llvm_name());
}

static PyObject*
comdat_get_target(PyObject* self, PyObject* args) {
  return get_py_handle(get_object<lb::Comdat>(parse_handle(args)).get_target());
}

static PyObject*
comdat_has_source_info(PyObject* self, PyObject* args) {
  return convert(false);
}

static PyObject*
comdat_is_artificial(PyObject* self, PyObject* args) {
  return convert(true);
}

// Function interface

static PyObject*
func_has_llvm_defn(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Function>(parse_handle(args)).has_llvm_defn());
}

static PyObject*
func_get_llvm_defn(PyObject* self, PyObject* args) {
  const auto& f = get_object<lb::Function>(parse_handle(args));
  if(f.has_llvm_defn())
    return get_py_handle(f.get_llvm_defn(), HandleKind::Definition);
  return get_py_handle();
}

static PyObject*
func_get_llvm_span(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Function>(parse_handle(args)).get_llvm_span());
}

static PyObject*
func_has_source_defn(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::Function>(parse_handle(args)).has_source_defn());
}

static PyObject*
func_get_source_defn(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::Function>(parse_handle(args)).get_source_defn());
}

static PyObject*
func_get_num_uses(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Function>(parse_handle(args)).get_num_uses());
}

static PyObject*
func_get_uses(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Function>(parse_handle(args)).uses());
}

static PyObject*
func_get_indirect_uses(PyObject* self, PyObject* args) {
  lb::error() << "UNIMPLEMENTED: function_get_indirect_uses()\n";
  return nullptr;
}

static PyObject*
func_get_tag(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Function>(parse_handle(args)).get_tag());
}

static PyObject*
func_get_llvm_name(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Function>(parse_handle(args)).get_llvm_name());
}

static PyObject*
func_get_source_name(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::Function>(parse_handle(args)).get_source_name());
}

static PyObject*
func_get_full_name(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Function>(parse_handle(args)).get_full_name());
}

static PyObject*
func_has_source_info(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::Function>(parse_handle(args)).has_source_info());
}

static PyObject*
func_is_artificial(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Function>(parse_handle(args)).is_artificial());
}

static PyObject*
func_is_mangled(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Function>(parse_handle(args)).is_mangled());
}

static PyObject*
func_get_args(PyObject* self, PyObject* args) {
  const auto& f = get_object<lb::Function>(parse_handle(args));
  PyObject* arguments   = PyList_New(0);
  for(const lb::Argument& arg : f.arguments())
    PyList_Append(arguments, get_py_handle(arg, HandleKind::Argument));

  Py_INCREF(arguments);
  return arguments;
}

static PyObject*
func_get_blocks(PyObject* self, PyObject* args) {
  const auto& f = get_object<lb::Function>(parse_handle(args));
  PyObject* blocks      = PyList_New(0);
  for(const lb::BasicBlock& bb : f.blocks())
    PyList_Append(blocks, get_py_handle(bb, HandleKind::BasicBlock));

  Py_INCREF(blocks);
  return blocks;
}

static PyObject*
func_get_comdat(PyObject* self, PyObject* args) {
  const auto& f = get_object<lb::Function>(parse_handle(args));
  if(const lb::Comdat* comdat = f.get_comdat())
    return get_py_handle(*comdat, HandleKind::Comdat);
  return get_py_handle();
}

// Global interface

static PyObject*
global_has_llvm_defn(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::GlobalVariable>(parse_handle(args)).has_llvm_defn());
}

static PyObject*
global_get_llvm_defn(PyObject* self, PyObject* args) {
  const auto& g = get_object<lb::GlobalVariable>(parse_handle(args));
  if(g.has_llvm_defn())
    return get_py_handle(g.get_llvm_defn(), HandleKind::Definition);
  return get_py_handle();
}

static PyObject*
global_get_llvm_span(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::GlobalVariable>(parse_handle(args)).get_llvm_span());
}

static PyObject*
global_has_source_defn(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::GlobalVariable>(parse_handle(args)).has_source_defn());
}

static PyObject*
global_get_source_defn(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::GlobalVariable>(parse_handle(args)).get_source_defn());
}

static PyObject*
global_get_num_uses(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::GlobalVariable>(parse_handle(args)).get_num_uses());
}

static PyObject*
global_get_uses(PyObject* self, PyObject* args) {
  return convert(get_object<lb::GlobalVariable>(parse_handle(args)).uses());
}

static PyObject*
global_get_indirect_uses(PyObject* self, PyObject* args) {
  lb::error() << "UNIMPLEMENTED: global_get_indirect_uses()\n";
  return nullptr;
}

static PyObject*
global_get_tag(PyObject* self, PyObject* args) {
  return convert(get_object<lb::GlobalVariable>(parse_handle(args)).get_tag());
}

static PyObject*
global_get_llvm_name(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::GlobalVariable>(parse_handle(args)).get_llvm_name());
}

static PyObject*
global_get_source_name(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::GlobalVariable>(parse_handle(args)).get_source_name());
}

static PyObject*
global_get_full_name(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::GlobalVariable>(parse_handle(args)).get_full_name());
}

static PyObject*
global_has_source_info(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::GlobalVariable>(parse_handle(args)).has_source_info());
}

static PyObject*
global_is_artificial(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::GlobalVariable>(parse_handle(args)).is_artificial());
}

static PyObject*
global_is_mangled(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::GlobalVariable>(parse_handle(args)).is_mangled());
}

static PyObject*
global_get_comdat(PyObject* self, PyObject* args) {
  const auto& g = get_object<lb::GlobalVariable>(parse_handle(args));
  if(const lb::Comdat* comdat = g.get_comdat())
    return get_py_handle(*comdat, HandleKind::Comdat);
  return get_py_handle();
}

// Instruction interface

static PyObject*
inst_has_llvm_defn(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::Instruction>(parse_handle(args)).has_llvm_defn());
}

static PyObject*
inst_get_llvm_defn(PyObject* self, PyObject* args) {
  const auto& inst = get_object<lb::Instruction>(parse_handle(args));
  if(inst.has_llvm_defn())
    return get_py_handle(inst.get_llvm_defn(), HandleKind::Definition);
  return get_py_handle();
}

static PyObject*
inst_get_llvm_span(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::Instruction>(parse_handle(args)).get_llvm_span());
}

static PyObject*
inst_has_source_defn(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::Instruction>(parse_handle(args)).has_source_defn());
}

static PyObject*
inst_get_source_defn(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::Instruction>(parse_handle(args)).get_source_defn());
}

static PyObject*
inst_get_num_uses(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::Instruction>(parse_handle(args)).get_num_uses());
}

static PyObject*
inst_get_uses(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Instruction>(parse_handle(args)).uses());
}

static PyObject*
inst_get_indirect_uses(PyObject* self, PyObject* args) {
  lb::error() << "UNIMPLEMENTED: inst_get_indirect_uses()\n";
  return nullptr;
}

static PyObject*
inst_get_tag(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Instruction>(parse_handle(args)).get_tag());
}

static PyObject*
inst_has_source_info(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::Instruction>(parse_handle(args)).has_source_info());
}

static PyObject*
inst_is_artificial(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Instruction>(parse_handle(args))
                     .get_function()
                     .is_artificial());
}

static PyObject*
inst_is_llvm_debug_inst(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::Instruction>(parse_handle(args)).is_llvm_debug_inst());
}

static PyObject*
inst_is_llvm_lifetime_inst(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::Instruction>(parse_handle(args)).is_llvm_lifetime_inst());
}

static PyObject*
inst_get_block(PyObject* self, PyObject* args) {
  return get_py_handle(
      get_object<lb::Instruction>(parse_handle(args)).get_block(),
      HandleKind::BasicBlock);
}

static PyObject*
inst_get_function(PyObject* self, PyObject* args) {
  return get_py_handle(
      get_object<lb::Instruction>(parse_handle(args)).get_function(),
      HandleKind::Function);
}

// Metadata interface

static PyObject*
md_has_llvm_defn(PyObject* self, PyObject* args) {
  return convert(get_object<lb::MDNode>(parse_handle(args)).has_llvm_defn());
}

static PyObject*
md_get_llvm_defn(PyObject* self, PyObject* args) {
  const auto& md = get_object<lb::MDNode>(parse_handle(args));
  if(md.has_llvm_defn())
    return get_py_handle(md.get_llvm_defn(), HandleKind::Definition);
  return get_py_handle();
}

static PyObject*
md_get_llvm_span(PyObject* self, PyObject* args) {
  return convert(get_object<lb::MDNode>(parse_handle(args)).get_llvm_span());
}

static PyObject*
md_get_num_uses(PyObject* self, PyObject* args) {
  return convert(get_object<lb::MDNode>(parse_handle(args)).get_num_uses());
}

static PyObject*
md_get_uses(PyObject* self, PyObject* args) {
  return convert(get_object<lb::MDNode>(parse_handle(args)).uses());
}

static PyObject*
md_get_indirect_uses(PyObject* self, PyObject* args) {
  return md_get_uses(self, args);
}

static PyObject*
md_get_tag(PyObject* self, PyObject* args) {
  return convert(get_object<lb::MDNode>(parse_handle(args)).get_tag());
}

static PyObject*
md_has_source_info(PyObject* self, PyObject* args) {
  return convert(false);
}

static PyObject*
md_is_artificial(PyObject* self, PyObject* args) {
  return convert(get_object<lb::MDNode>(parse_handle(args)).is_artificial());
}

// Struct interface

static PyObject*
struct_has_llvm_defn(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::StructType>(parse_handle(args)).has_llvm_defn());
}

static PyObject*
struct_get_llvm_defn(PyObject* self, PyObject* args) {
  const auto& s = get_object<lb::StructType>(parse_handle(args));
  if(s.has_llvm_defn())
    return get_py_handle(s.get_llvm_defn(), HandleKind::Definition);
  return get_py_handle();
}

static PyObject*
struct_get_llvm_span(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::StructType>(parse_handle(args)).get_llvm_span());
}

static PyObject*
struct_has_source_defn(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::StructType>(parse_handle(args)).has_source_defn());
}

static PyObject*
struct_get_source_defn(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::StructType>(parse_handle(args)).get_source_defn());
}

static PyObject*
struct_get_num_uses(PyObject* self, PyObject* args) {
  return convert(get_object<lb::StructType>(parse_handle(args)).get_num_uses());
}

static PyObject*
struct_get_uses(PyObject* self, PyObject* args) {
  return convert(get_object<lb::StructType>(parse_handle(args)).uses());
}

static PyObject*
struct_get_indirect_uses(PyObject* self, PyObject* args) {
  // Indirect uses don't make much sense for a struct type, so just return
  // the uses
  return struct_get_uses(self, args);
}

static PyObject*
struct_get_tag(PyObject* self, PyObject* args) {
  return convert(get_object<lb::StructType>(parse_handle(args)).get_tag());
}

static PyObject*
struct_get_llvm_name(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::StructType>(parse_handle(args)).get_llvm_name());
}

static PyObject*
struct_get_source_name(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::StructType>(parse_handle(args)).get_source_name());
}

static PyObject*
struct_get_full_name(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::StructType>(parse_handle(args)).get_full_name());
}

static PyObject*
struct_has_source_info(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::StructType>(parse_handle(args)).has_source_info());
}

static PyObject*
struct_is_artificial(PyObject* self, PyObject* args) {
  return convert(
      get_object<lb::StructType>(parse_handle(args)).is_artificial());
}

// Use interface

static PyObject*
use_get_begin(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Use>(parse_handle(args)).get_begin());
}

static PyObject*
use_get_end(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Use>(parse_handle(args)).get_end());
}

static PyObject*
use_get_instruction(PyObject* self, PyObject* args) {
  const auto& use = get_object<lb::Use>(parse_handle(args));
  if(const lb::Instruction* inst = use.get_instruction())
    return get_py_handle(*inst, HandleKind::Instruction);
  return get_py_handle();
}

static PyObject*
use_get_used(PyObject* self, PyObject* args) {
  return get_py_handle(get_object<lb::Use>(parse_handle(args)).get_used());
}

// Definition interface

static PyObject*
def_get_begin(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Definition>(parse_handle(args)).get_begin());
}

static PyObject*
def_get_end(PyObject* self, PyObject* args) {
  return convert(get_object<lb::Definition>(parse_handle(args)).get_end());
}

static PyObject*
def_get_defined(PyObject* self, PyObject* args) {
  return get_py_handle(
      get_object<lb::Definition>(parse_handle(args)).get_defined());
}

// Generic interface

static PyObject*
entity_has_llvm_defn(PyObject* self, PyObject* args) {
  Handle handle = parse_handle(args);
  switch(get_handle_kind(handle)) {
  case HandleKind::Argument:
    return arg_has_llvm_defn(self, args);
  case HandleKind::BasicBlock:
    return block_has_llvm_defn(self, args);
  case HandleKind::Comdat:
    return comdat_has_llvm_defn(self, args);
  case HandleKind::Function:
    return func_has_llvm_defn(self, args);
  case HandleKind::GlobalAlias:
    return alias_has_llvm_defn(self, args);
  case HandleKind::GlobalVariable:
    return global_has_llvm_defn(self, args);
  case HandleKind::Instruction:
    return inst_has_llvm_defn(self, args);
  case HandleKind::MDNode:
    return md_has_llvm_defn(self, args);
  case HandleKind::StructType:
    return struct_has_llvm_defn(self, args);
  default:
    lb::warning() << "Cannot check if entity has LLVM definition: "
                  << get_handle_kind_name(handle) << "\n";
    break;
  }

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject*
entity_get_llvm_defn(PyObject* self, PyObject* args) {
  Handle handle = parse_handle(args);
  switch(get_handle_kind(handle)) {
  case HandleKind::Argument:
    return arg_get_llvm_defn(self, args);
  case HandleKind::BasicBlock:
    return block_get_llvm_defn(self, args);
  case HandleKind::Comdat:
    return comdat_get_llvm_defn(self, args);
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
  case HandleKind::Comdat:
    return comdat_get_llvm_span(self, args);
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
entity_has_source_defn(PyObject* self, PyObject* args) {
  Handle handle = parse_handle(args);
  switch(get_handle_kind(handle)) {
    case HandleKind::Argument:
    return arg_has_source_defn(self, args);
    case HandleKind::BasicBlock:
    return block_has_source_defn(self, args);
    case HandleKind::Function:
    return func_has_source_defn(self, args);
    case HandleKind::GlobalVariable:
    return global_has_source_defn(self, args);
    case HandleKind::Instruction:
    return global_has_source_defn(self, args);
    case HandleKind::StructType:
    return struct_has_source_defn(self, args);
    default:
    lb::warning() << "Could not check if entity has source definition: "
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
  case HandleKind::BasicBlock:
    return block_get_source_defn(self, args);
  case HandleKind::Function:
    return func_get_source_defn(self, args);
  case HandleKind::GlobalVariable:
    return global_get_source_defn(self, args);
  case HandleKind::Instruction:
    return inst_get_source_defn(self, args);
  case HandleKind::StructType:
    return struct_get_source_defn(self, args);
  default:
    lb::warning() << "Cannot get LLVM definition for entity: "
                  << get_handle_kind_name(handle) << "\n";
    break;
  }

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject*
entity_get_num_uses(PyObject* self, PyObject* args) {
  Handle handle = parse_handle(args);
  switch(get_handle_kind(handle)) {
  case HandleKind::Argument:
    return arg_get_num_uses(self, args);
  case HandleKind::BasicBlock:
    return block_get_num_uses(self, args);
  case HandleKind::Function:
    return func_get_num_uses(self, args);
  case HandleKind::GlobalAlias:
    return alias_get_num_uses(self, args);
  case HandleKind::GlobalVariable:
    return global_get_num_uses(self, args);
  case HandleKind::Instruction:
    return global_get_num_uses(self, args);
  case HandleKind::MDNode:
    return md_get_num_uses(self, args);
  case HandleKind::StructType:
    return struct_get_num_uses(self, args);
  default:
    lb::error() << "Cannot get uses for entity: "
                << get_handle_kind_name(handle) << "\n";
    return nullptr;
  }

  return PyList_New(0);
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
  case HandleKind::Comdat:
    return comdat_get_tag(self, args);
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
  case HandleKind::Comdat:
    return comdat_get_llvm_name(self, args);
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
entity_has_source_info(PyObject* self, PyObject* args) {
  Handle handle = parse_handle(args);
  switch(get_handle_kind(handle)) {
  case HandleKind::Argument:
    return arg_has_source_info(self, args);
  case HandleKind::BasicBlock:
    return block_has_source_info(self, args);
  case HandleKind::Comdat:
    return comdat_has_source_info(self, args);
  case HandleKind::Function:
    return func_has_source_info(self, args);
  case HandleKind::GlobalAlias:
    return alias_has_source_info(self, args);
  case HandleKind::GlobalVariable:
    return global_has_source_info(self, args);
  case HandleKind::Instruction:
    return inst_has_source_info(self, args);
  case HandleKind::MDNode:
    return md_has_source_info(self, args);
  case HandleKind::StructType:
    return struct_has_source_info(self, args);
  default:
    lb::error() << "Cannot check if entity has source info: "
                << get_handle_kind_name(handle) << "\n";
    return nullptr;
  }
}

static PyObject*
entity_is_artificial(PyObject* self, PyObject* args) {
  Handle handle = parse_handle(args);
  switch(get_handle_kind(handle)) {
  case HandleKind::Argument:
    return arg_is_artificial(self, args);
  case HandleKind::BasicBlock:
    return block_is_artificial(self, args);
  case HandleKind::Comdat:
    return comdat_is_artificial(self, args);
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

static PyObject*
entity_get_kind_name(PyObject* self, PyObject* args) {
  return convert(get_handle_kind_name(parse_handle(args)));
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
    FUNC(is_use, "True if the handle is a use"),
    FUNC(is_def, "True if the handle is a definition"),
    FUNC(is_null_handle, "True if the handle is None"),
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
    FUNC(module_get_def_at, "Gets the definition at the offset or HANDLE_NULL"),
    FUNC(module_get_use_at, "Gets the use at the offset or HANDLE_NULL"),
    FUNC(module_get_comdat_at, "Gets the comdat at the offset or HANDLE_NULL"),
    FUNC(module_get_function_at,
         "Gets the function at the offset or HANDLE_NULL"),
    FUNC(module_get_block_at, "Gets the block at the offset or HANDLE_NULL"),
    FUNC(module_get_instruction_at,
         "Gets the instruction at the offset or HANDLE_NULL"),

    // Alias interface
    FUNC(alias_has_llvm_defn, "Check if the alias has an LLVM definition"),
    FUNC(alias_get_llvm_defn, "LLVM definition range of the alias"),
    FUNC(alias_get_llvm_span, "LLVM span of the alias"),
    FUNC(alias_get_num_uses, "Number of uses of the alias"),
    FUNC(alias_get_uses, "Uses of the alias"),
    FUNC(alias_get_indirect_uses, "Indirect uses of the alias"),
    FUNC(alias_get_tag, "Tag of the alias"),
    FUNC(alias_get_llvm_name, "LLVM name of the alias"),
    FUNC(alias_has_source_info, "Always returns false"),
    FUNC(alias_is_artificial, "Always returns true"),

    // Argument interface
    FUNC(arg_has_llvm_defn, "Check if the argument has an LLVM definition"),
    FUNC(arg_get_llvm_defn, "LLVM definition range of the argument"),
    FUNC(arg_get_llvm_span, "LLVM span of the argument"),
    FUNC(arg_has_source_defn, "Check if the argument has a source location"),
    FUNC(arg_get_source_defn, "Source location of the argument"),
    FUNC(arg_get_num_uses, "Number of uses of the argument"),
    FUNC(arg_get_uses, "Uses of the argument"),
    FUNC(arg_get_indirect_uses, "Indirect uses of the argument"),
    FUNC(arg_get_tag, "Tag of the argument"),
    FUNC(arg_get_llvm_name, "LLVM name of the argument"),
    FUNC(arg_get_source_name, "Source name of the argument"),
    FUNC(arg_has_source_info,
         "True if there is source information attached to the argument"),
    FUNC(arg_is_artificial,
         "True if the argument was inserted by the compiler"),

    // Basic block interface
    FUNC(block_has_llvm_defn,
         "Check if the basic block has an LLVM definition"),
    FUNC(block_get_llvm_defn, "LLVM definition range of the basic block"),
    FUNC(block_get_llvm_span, "LLVM span of the basic block"),
    FUNC(block_has_source_defn,
         "Check if the basic block has a source location"),
    FUNC(block_get_source_defn, "Gets the source location of the argument"),
    FUNC(block_get_num_uses, "Number of uses of the block"),
    FUNC(block_get_uses, "Uses of the block"),
    FUNC(block_get_indirect_uses, "Indirect uses of the block"),
    FUNC(block_get_tag, "Tag of the basic block"),
    FUNC(block_has_source_info,
         "True if the block has source information attached"),
    FUNC(block_is_artificial, "True if the parent function is artificial"),
    FUNC(block_get_instructions,
         "List of handls to the instructions in the basic block"),

    // Comdat interface
    FUNC(comdat_has_llvm_defn, "Check if the comdat has an LLVM definition"),
    FUNC(comdat_get_llvm_defn, "LLVM definition range of the comdat"),
    FUNC(comdat_get_self_llvm_defn, "LLVM definition of the comdat itself"),
    FUNC(comdat_get_llvm_span, "LLVM span of the comdat"),
    FUNC(comdat_get_tag, "Tag of the comdat"),
    FUNC(comdat_get_llvm_name, "LLVM name of the comdat"),
    FUNC(comdat_get_target, "Target of the comdat"),
    FUNC(comdat_has_source_info, "Always returns false"),
    FUNC(comdat_is_artificial, "Always returns true"),

    // Function interface
    FUNC(func_has_llvm_defn, "Check if the function has an LLVM definition"),
    FUNC(func_get_llvm_defn, "LLVM definition range of the function"),
    FUNC(func_get_llvm_span, "LLVM span of the function"),
    FUNC(func_has_source_defn, "Check if the function has a source location"),
    FUNC(func_get_source_defn, "Source location of the function"),
    FUNC(func_get_num_uses, "Number of uses of the function"),
    FUNC(func_get_uses, "Uses of the function"),
    FUNC(func_get_indirect_uses, "Indirect uses of the function"),
    FUNC(func_get_tag, "Tag of the function"),
    FUNC(func_get_llvm_name, "LLVM name of the function"),
    FUNC(func_get_source_name, "Source name of the function"),
    FUNC(func_get_full_name, "Full name of the function"),
    FUNC(func_has_source_info,
         "True if the function has source information attached"),
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
    FUNC(global_has_llvm_defn, "Check if the global has an LLVM definition"),
    FUNC(global_get_llvm_defn, "LLVM definition range of the global"),
    FUNC(global_get_llvm_span, "LLVM span of the global"),
    FUNC(global_has_source_defn,
         "Check if the global variable has a source location"),
    FUNC(global_get_source_defn, "Source location of the global"),
    FUNC(global_get_num_uses, "Number of uses of the global"),
    FUNC(global_get_uses, "Uses of the global"),
    FUNC(global_get_indirect_uses, "Indirect uses of the global"),
    FUNC(global_get_tag, "Tag of the global"),
    FUNC(global_get_llvm_name, "LLVM name of the global"),
    FUNC(global_get_source_name, "Source name of the global"),
    FUNC(global_has_source_info,
         "True if the global variable has source information attached"),
    FUNC(global_is_artificial,
         "True if the global was generated by the compiler"),
    FUNC(global_is_mangled, "True if the LLVM name of the global is mangled"),
    FUNC(global_get_comdat,
         "Handle to the comdat of the global variable or HANDLE_NULL if the "
         "global does not have a comdat"),

    // Instruction interface
    FUNC(inst_has_llvm_defn, "Check if the instruction has an LLVM definition"),
    FUNC(inst_get_llvm_defn, "LLVM definition range of the instruction"),
    FUNC(inst_get_llvm_span, "LLVM span of the instruction"),
    FUNC(inst_has_source_defn,
         "Check if the instruction has a source location"),
    FUNC(inst_get_source_defn, "Source location of the instruction"),
    FUNC(inst_get_num_uses, "Number of uses of the instruction"),
    FUNC(inst_get_uses, "Uses of the instruction"),
    FUNC(inst_get_indirect_uses, "Indirect uses of the instruction"),
    FUNC(inst_get_tag, "Tag of the instruction (may be a LLVM slot)"),
    FUNC(inst_has_source_info,
         "True if the instruction has source information attached"),
    FUNC(inst_is_artificial, "True if the parent function is artificial"),
    FUNC(inst_is_llvm_debug_inst,
         "True if the instruction is a call to an LLVM debug intrinsic"),
    FUNC(inst_is_llvm_lifetime_inst,
         "True if the instruction is a call to an LLVM memory lifetime "
         "intrinsic"),
    FUNC(inst_get_function, "The function to which this instruction belongs"),
    FUNC(inst_get_block, "The basic block to which this instruction belongs"),

    // Metadata interface
    FUNC(md_has_llvm_defn, "Check if the metadata node has an LLVM definition"),
    FUNC(md_get_llvm_defn, "LLVM definition range of the metadata node"),
    FUNC(md_get_llvm_span, "LLVM span of the metadata node"),
    FUNC(md_get_num_uses, "Number of uses of the metadata node"),
    FUNC(md_get_uses, "Uses of the metadata node"),
    FUNC(md_get_indirect_uses, "Indirect uses of the metadata node"),
    FUNC(md_get_tag, "Tag of the metadata node"),
    FUNC(md_has_source_info, "Always returns false"),
    FUNC(md_is_artificial, "Always returns true"),

    // Struct interface
    FUNC(struct_has_llvm_defn, "Check if the struct has an LLVM definition"),
    FUNC(struct_get_llvm_defn, "LLVM definition range of the struct"),
    FUNC(struct_get_llvm_span, "LLVM span of the struct"),
    FUNC(struct_has_source_defn, "Check if the struct has a source location"),
    FUNC(struct_get_source_defn, "Source location of the struct"),
    FUNC(struct_get_num_uses, "Number of uses of the struct"),
    FUNC(struct_get_uses, "Uses of the struct"),
    FUNC(struct_get_indirect_uses, "Indirect uses of the struct"),
    FUNC(struct_get_tag, "Tag of the struct"),
    FUNC(struct_get_llvm_name, "LLVM name of the struct"),
    FUNC(struct_get_source_name, "Source name of the struct"),
    FUNC(struct_get_full_name, "Full name of the struct"),
    FUNC(struct_has_source_info,
         "True if the struct has source information attached"),
    FUNC(struct_is_artificial,
         "True if the struct was generated by the compiler"),

    // Use interface
    FUNC(use_get_begin, "Get the start offset of the use"),
    FUNC(use_get_end, "Get the end offset of the use"),
    FUNC(use_get_instruction, "The instruction (if any) of the use"),
    FUNC(use_get_used, "The entity represented by the use"),

    // Definition interface
    FUNC(def_get_begin, "Get the start offset of the definition"),
    FUNC(def_get_end, "Get the end offset of the definition"),
    FUNC(def_get_defined, "The entity defined by the definition"),

    // Generic interface
    FUNC(entity_has_llvm_defn, "True if the entity has an LLVM definition set"),
    FUNC(entity_get_llvm_defn, "LLVM definition range of the entity"),
    FUNC(entity_get_llvm_span, "LLVM span of the entity"),
    FUNC(entity_has_source_defn, "Check if the entity has a source location"),
    FUNC(entity_get_source_defn, "Source location of the entity"),
    FUNC(entity_get_num_uses, "Number of uses of the entity"),
    FUNC(entity_get_uses, "Uses of the entity"),
    FUNC(entity_get_indirect_uses, "Indirect uses of the entity"),
    FUNC(entity_get_tag, "Tag of the entity"),
    FUNC(entity_get_llvm_name, "LLVM name of the entity"),
    FUNC(entity_get_source_name, "Source name of the entity"),
    FUNC(entity_get_full_name, "Full name of the entity"),
    FUNC(entity_has_source_info,
         "True if the entity has source information attached"),
    FUNC(entity_is_artificial,
         "True if the entity was generated by the compiler"),
    FUNC(entity_get_kind_name, "The entity kind"),

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
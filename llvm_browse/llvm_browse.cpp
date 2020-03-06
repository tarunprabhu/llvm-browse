#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "lib/Argument.h"
#include "lib/BasicBlock.h"
#include "lib/Function.h"
#include "lib/GlobalAlias.h"
#include "lib/GlobalVariable.h"
#include "lib/Logging.h"
#include "lib/MDNode.h"
#include "lib/Module.h"
#include "lib/StructType.h"

#include <cstdio>
#include <type_traits>

using Handle = uint64_t;

// The tags are added to the Handle to be able to determine the dynamic type
// of the object from the handle
enum class HandleKind {
  Module         = 0x0,
  GlobalAlias    = 0x1,
  Function       = 0x2,
  GlobalVariable = 0x3,
  StructType     = 0x4,
  MDNode         = 0x5,
  LLVMRange      = 0x6,
  SourceRange    = 0x7,
  Argument       = 0x8,
  BasicBlock     = 0x9,
  Mask           = 0xf,
};

static const Handle HandleInvalid = 0;
static const char* HandleFmt      = "k";

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

static PyTypeObject* PySourcePoint = nullptr;
static PyTypeObject* PySourceRange = nullptr;
static PyTypeObject* PyLLVMRange   = nullptr;

static PyObject*
convert(llvm::StringRef s) {
  if(s.size())
    return PyUnicode_FromString(s.data());
  return PyUnicode_FromString("");
}

static PyObject*
convert(const lb::SourceRange& range) {
  PyObject* py_begin = PyStructSequence_New(PySourcePoint);
  PyStructSequence_SetItem(py_begin, 0, PyLong_FromLong(range.begin.line));
  PyStructSequence_SetItem(py_begin, 1, PyLong_FromLong(range.begin.column));
  Py_INCREF(py_begin);

  PyObject* py_end = PyStructSequence_New(PySourcePoint);
  PyStructSequence_SetItem(py_end, 0, PyLong_FromLong(range.end.line));
  PyStructSequence_SetItem(py_end, 1, PyLong_FromLong(range.end.column));
  Py_INCREF(py_end);

  PyObject* py = PyStructSequence_New(PySourceRange);
  if(range.file)
    PyStructSequence_SetItem(py, 0, PyUnicode_FromString(range.file));
  else
    PyStructSequence_SetItem(py, 0, PyUnicode_FromString(""));
  PyStructSequence_SetItem(py, 1, py_begin);
  PyStructSequence_SetItem(py, 2, py_end);
  Py_INCREF(py);

  return py;
}

static PyObject*
convert(const lb::LLVMRange& range) {
  PyObject* py = PyStructSequence_New(PyLLVMRange);
  PyStructSequence_SetItem(py, 0, PyLong_FromLong(range.begin));
  PyStructSequence_SetItem(py, 1, PyLong_FromLong(range.end));
  Py_INCREF(py);

  return py;
}

static PyObject*
module_create(PyObject* self, PyObject* args) {
  const char* file;

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
  Handle handle = HandleInvalid;

  if(!PyArg_ParseTuple(args, HandleFmt, &handle))
    return nullptr;

  const lb::Module* module = get_object<lb::Module>(handle);
  return convert(module->get_code());
}

static PyObject*
module_get_aliases(PyObject* self, PyObject* args) {
  Handle handle = HandleInvalid;

  if(!PyArg_ParseTuple(args, HandleFmt, &handle))
    return nullptr;

  const lb::Module* module = get_object<lb::Module>(handle);
  PyObject* list           = PyList_New(0);
  for(const lb::GlobalAlias* alias : module->aliases())
    PyList_Append(list, get_py_handle(alias, HandleKind::GlobalAlias));
  return list;
}

static PyObject*
module_get_functions(PyObject* self, PyObject* args) {
  Handle handle = HandleInvalid;
  if(!PyArg_ParseTuple(args, HandleFmt, &handle))
    return nullptr;

  const lb::Module* module = get_object<lb::Module>(handle);
  PyObject* list           = PyList_New(0);
  for(const lb::Function* f : module->functions())
    PyList_Append(list, get_py_handle(f, HandleKind::Function));
  return list;
}

static PyObject*
module_get_globals(PyObject* self, PyObject* args) {
  Handle handle = HandleInvalid;
  if(!PyArg_ParseTuple(args, HandleFmt, &handle))
    return nullptr;

  const lb::Module* module = get_object<lb::Module>(handle);
  PyObject* list           = PyList_New(0);
  for(const lb::GlobalVariable* g : module->globals())
    PyList_Append(list, get_py_handle(g, HandleKind::GlobalVariable));
  return list;
}

static PyObject*
module_get_structs(PyObject* self, PyObject* args) {
  Handle handle = HandleInvalid;
  if(!PyArg_ParseTuple(args, HandleFmt, &handle))
    return nullptr;

  const lb::Module* module = get_object<lb::Module>(handle);
  PyObject* list           = PyList_New(0);
  for(const lb::StructType* s : module->structs())
    PyList_Append(list, get_py_handle(s, HandleKind::StructType));
  return list;
}

static PyObject*
module_free(PyObject* self, PyObject* args) {
  Handle handle = HandleInvalid;
  if(!PyArg_ParseTuple(args, HandleFmt, &handle))
    return nullptr;

  delete get_object<lb::Module>(handle);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject*
entity_get_tag(PyObject* self, PyObject* args) {
  Handle handle = HandleInvalid;
  if(!PyArg_ParseTuple(args, HandleFmt, &handle))
    return nullptr;

  switch(get_handle_kind(handle)) {
  case HandleKind::Argument:
    return convert(get_object<lb::Argument>(handle)->get_tag());
  case HandleKind::BasicBlock:
    return convert(get_object<lb::BasicBlock>(handle)->get_tag());
  case HandleKind::GlobalAlias:
    return convert(get_object<lb::GlobalAlias>(handle)->get_tag());
  case HandleKind::Function:
    return convert(get_object<lb::Function>(handle)->get_tag());
  case HandleKind::GlobalVariable:
    return convert(get_object<lb::GlobalVariable>(handle)->get_tag());
  case HandleKind::MDNode:
    return convert(get_object<lb::MDNode>(handle)->get_tag());
  case HandleKind::StructType:
    return convert(get_object<lb::StructType>(handle)->get_tag());
  default:
    break;
  }

  return convert(llvm::StringRef());
}

static PyObject*
entity_has_source_info(PyObject* self, PyObject* args) {
  Handle handle = HandleInvalid;
  if(!PyArg_ParseTuple(args, HandleFmt, &handle))
    return nullptr;

  switch(get_handle_kind(handle)) {
  case HandleKind::Function:
    return PyBool_FromLong(get_object<lb::Function>(handle)->has_source_info());
  case HandleKind::GlobalVariable:
    return PyBool_FromLong(
        get_object<lb::GlobalVariable>(handle)->has_source_info());
  case HandleKind::StructType:
    return PyBool_FromLong(
        get_object<lb::StructType>(handle)->has_source_info());
  default:
    break;
  }
  return Py_False;
}

static PyObject*
entity_get_source_name(PyObject* self, PyObject* args) {
  Handle handle = HandleInvalid;
  if(!PyArg_ParseTuple(args, HandleFmt, &handle))
    return nullptr;

  switch(get_handle_kind(handle)) {
  case HandleKind::Argument:
    return convert(get_object<lb::Argument>(handle)->get_source_name());
  case HandleKind::Function:
    return convert(get_object<lb::Function>(handle)->get_source_name());
  case HandleKind::GlobalVariable:
    return convert(get_object<lb::GlobalVariable>(handle)->get_source_name());
  case HandleKind::StructType:
    return convert(get_object<lb::StructType>(handle)->get_source_name());
  default:
    break;
  }

  return PyUnicode_FromString("");
}

static PyObject*
entity_get_llvm_name(PyObject* self, PyObject* args) {
  Handle handle = HandleInvalid;
  if(!PyArg_ParseTuple(args, HandleFmt, &handle))
    return nullptr;

  switch(get_handle_kind(handle)) {
  case HandleKind::Argument:
    return convert(get_object<lb::Argument>(handle)->get_llvm_name());
  case HandleKind::GlobalAlias:
    return convert(get_object<lb::GlobalAlias>(handle)->get_llvm_name());
  case HandleKind::Function:
    return convert(get_object<lb::Function>(handle)->get_llvm_name());
  case HandleKind::GlobalVariable:
    return convert(get_object<lb::GlobalVariable>(handle)->get_llvm_name());
  case HandleKind::StructType:
    return convert(get_object<lb::StructType>(handle)->get_llvm_name());
  default:
    break;
  }

  return PyUnicode_FromString("");
}

static PyObject*
entity_get_uses(PyObject* self, PyObject* args) {
  Handle handle = HandleInvalid;
  if(!PyArg_ParseTuple(args, HandleFmt, &handle))
    return nullptr;

  PyObject* uses = PyList_New(0);
  switch(get_handle_kind(handle)) {
  case HandleKind::Argument:
    for(const lb::LLVMRange& r : get_object<lb::Argument>(handle)->uses())
      PyList_Append(uses, convert(r));
    break;
  case HandleKind::GlobalAlias:
    for(const lb::LLVMRange& r : get_object<lb::GlobalAlias>(handle)->uses())
      PyList_Append(uses, convert(r));
    break;
  case HandleKind::Function:
    for(const lb::LLVMRange& r : get_object<lb::Function>(handle)->uses())
      PyList_Append(uses, convert(r));
    break;
  case HandleKind::GlobalVariable:
    for(const lb::LLVMRange& r : get_object<lb::GlobalVariable>(handle)->uses())
      PyList_Append(uses, convert(r));
    break;
  case HandleKind::MDNode:
    for(const lb::LLVMRange& r : get_object<lb::MDNode>(handle)->uses())
      PyList_Append(uses, convert(r));
    break;
  case HandleKind::StructType:
    for(const lb::LLVMRange& r : get_object<lb::StructType>(handle)->uses())
      PyList_Append(uses, convert(r));
    break;
  default:
    break;
  }

  return uses;
}

static PyObject*
entity_get_llvm_defn(PyObject* self, PyObject* args) {
  Handle handle = HandleInvalid;
  if(!PyArg_ParseTuple(args, HandleFmt, &handle))
    return nullptr;

  switch(get_handle_kind(handle)) {
  case HandleKind::GlobalAlias:
    return convert(get_object<lb::GlobalAlias>(handle)->get_llvm_defn());
  case HandleKind::Function:
    return convert(get_object<lb::Function>(handle)->get_llvm_defn());
  case HandleKind::GlobalVariable:
    return convert(get_object<lb::GlobalVariable>(handle)->get_llvm_defn());
  case HandleKind::MDNode:
    return convert(get_object<lb::MDNode>(handle)->get_llvm_defn());
  case HandleKind::StructType:
    return convert(get_object<lb::StructType>(handle)->get_llvm_defn());
  default:
    break;
  }

  return convert(lb::LLVMRange());
}

static PyObject*
entity_get_llvm_span(PyObject* self, PyObject* args) {
  Handle handle = HandleInvalid;
  if(!PyArg_ParseTuple(args, HandleFmt, &handle))
    return nullptr;

  switch(get_handle_kind(handle)) {
  case HandleKind::GlobalAlias:
    return convert(get_object<lb::GlobalAlias>(handle)->get_llvm_span());
  case HandleKind::Function:
    return convert(get_object<lb::Function>(handle)->get_llvm_span());
  case HandleKind::GlobalVariable:
    return convert(get_object<lb::GlobalVariable>(handle)->get_llvm_span());
  case HandleKind::MDNode:
    return convert(get_object<lb::MDNode>(handle)->get_llvm_span());
  case HandleKind::StructType:
    return convert(get_object<lb::StructType>(handle)->get_llvm_span());
  default:
    break;
  }

  return convert(lb::LLVMRange());
}

static PyObject*
entity_get_source_defn(PyObject* self, PyObject* args) {
  Handle handle = HandleInvalid;
  if(!PyArg_ParseTuple(args, HandleFmt, &handle))
    return nullptr;

  switch(get_handle_kind(handle)) {
  case HandleKind::GlobalAlias:
    return convert(get_object<lb::GlobalAlias>(handle)->get_source_defn());
  case HandleKind::Function:
    return convert(get_object<lb::Function>(handle)->get_source_defn());
  case HandleKind::GlobalVariable:
    return convert(get_object<lb::GlobalVariable>(handle)->get_source_defn());
  case HandleKind::StructType:
    return convert(get_object<lb::StructType>(handle)->get_source_defn());
  default:
    break;
  }

  return convert(lb::SourceRange());
}

static PyObject*
entity_get_source_span(PyObject* self, PyObject* args) {
  Handle handle = HandleInvalid;
  if(!PyArg_ParseTuple(args, HandleFmt, &handle))
    return nullptr;

  switch(get_handle_kind(handle)) {
  case HandleKind::GlobalAlias:
    return convert(get_object<lb::GlobalAlias>(handle)->get_source_span());
  case HandleKind::Function:
    return convert(get_object<lb::Function>(handle)->get_source_span());
  case HandleKind::GlobalVariable:
    return convert(get_object<lb::GlobalVariable>(handle)->get_source_span());
  case HandleKind::StructType:
    return convert(get_object<lb::StructType>(handle)->get_source_span());
  default:
    break;
  }

  return convert(lb::SourceRange());
}

static PyObject*
argument_is_artificial(PyObject* self, PyObject* args) {
  Handle handle = HandleInvalid;
  if(!PyArg_ParseTuple(args, HandleFmt, &handle))
    return nullptr;

  return PyBool_FromLong(get_object<lb::Argument>(handle)->is_artificial());
}

static PyObject*
function_get_full_name(PyObject* self, PyObject* args) {
  Handle handle = HandleInvalid;
  if(!PyArg_ParseTuple(args, HandleFmt, &handle))
    return nullptr;

  return convert(get_object<lb::Function>(handle)->get_full_name());
}

static PyObject*
function_is_artificial(PyObject* self, PyObject* args) {
  Handle handle = HandleInvalid;
  if(!PyArg_ParseTuple(args, HandleFmt, &handle))
    return nullptr;

  return PyBool_FromLong(get_object<lb::Function>(handle)->is_artificial());
}

static PyObject*
function_is_mangled(PyObject* self, PyObject* args) {
  Handle handle = HandleInvalid;
  if(!PyArg_ParseTuple(args, HandleFmt, &handle))
    return nullptr;

  return PyBool_FromLong(get_object<lb::Function>(handle)->is_mangled());
}

static PyObject*
global_get_full_name(PyObject* self, PyObject* args) {
  Handle handle = HandleInvalid;
  if(!PyArg_ParseTuple(args, HandleFmt, &handle))
    return nullptr;

  return convert(get_object<lb::GlobalVariable>(handle)->get_full_name());
}

static PyObject*
global_is_mangled(PyObject* self, PyObject* args) {
  Handle handle = HandleInvalid;
  if(!PyArg_ParseTuple(args, HandleFmt, &handle))
    return nullptr;

  return PyBool_FromLong(get_object<lb::GlobalVariable>(handle)->is_mangled());
}

static PyObject*
struct_get_full_name(PyObject* self, PyObject* args) {
  Handle handle = HandleInvalid;
  if(!PyArg_ParseTuple(args, HandleFmt, &handle))
    return nullptr;

  return convert(get_object<lb::StructType>(handle)->get_full_name());
}

static PyMethodDef module_methods[] = {
    {"module_create",
     (PyCFunction)module_create,
     METH_VARARGS,
     "Create a new module and return a handle to it"},

    {"module_free",
     (PyCFunction)module_free,
     METH_VARARGS,
     "Frees a module created by module_create"},

    {"module_get_code",
     (PyCFunction)module_get_code,
     METH_VARARGS,
     "Gets the LLVM IR for the module"},

    {"module_get_aliases",
     (PyCFunction)module_get_aliases,
     METH_VARARGS,
     "Gets the aliases from the module"},

    {"module_get_functions",
     (PyCFunction)module_get_functions,
     METH_VARARGS,
     "Gets the functions from the module"},

    {"module_get_globals",
     (PyCFunction)module_get_globals,
     METH_VARARGS,
     "Gets the globals from the module"},

    {"module_get_structs",
     (PyCFunction)module_get_structs,
     METH_VARARGS,
     "Gets the struct types in the module"},

    {"entity_get_tag",
     (PyCFunction)entity_get_tag,
     METH_VARARGS,
     "Gets the tag for the entity. This may be the same as the LLVM name with "
     "a pre-pended sentinel or a slot number with a prepended %"},

    {"entity_has_source_info",
     (PyCFunction)entity_has_source_info,
     METH_VARARGS,
     "Checks if the entity has source information associated with it"},

    {"entity_get_llvm_name",
     (PyCFunction)entity_get_llvm_name,
     METH_VARARGS,
     "Gets the LLVM name of the entity. This could be a slot"},

    {"entity_get_source_name",
     (PyCFunction)entity_get_source_name,
     METH_VARARGS,
     "Gets the source name of the entity"},

    {"entity_get_uses",
     (PyCFunction)entity_get_uses,
     METH_VARARGS,
     "Gets the uses of the entity. These will be LLVMRange's"},

    {"entity_get_llvm_defn",
     (PyCFunction)entity_get_llvm_defn,
     METH_VARARGS,
     "Gets the LLVM definition of the entity"},

    {"entity_get_llvm_span",
     (PyCFunction)entity_get_llvm_span,
     METH_VARARGS,
     "Gets the range that the entity spans in the LLVM IR"},

    {"entity_get_source_defn",
     (PyCFunction)entity_get_source_defn,
     METH_VARARGS,
     "Gets tne source definition of the entity"},

    {"entity_get_source_span",
     (PyCFunction)entity_get_source_span,
     METH_VARARGS,
     "Gets the range that the entity spans in the source"},

    {"argument_is_artificial",
     (PyCFunction)argument_is_artificial,
     METH_VARARGS,
     "True if the argument was implicitly added by the compiler"},

    {"function_get_full_name",
     (PyCFunction)function_get_full_name,
     METH_VARARGS,
     "Get the full name of the function"},

    {"function_is_artificial",
     (PyCFunction)function_is_artificial,
     METH_VARARGS,
     "True if the function was added by the compiler without a corresponding "
     "entry in the source"},

    {"function_is_mangled",
     (PyCFunction)function_is_mangled,
     METH_VARARGS,
     "Check if the function has a mangled name"},

    {"global_get_full_name",
     (PyCFunction)global_is_mangled,
     METH_VARARGS,
     "Get the full name of the global"},

    {"global_is_mangled",
     (PyCFunction)global_get_full_name,
     METH_VARARGS,
     "Check if the global has a mangled name"},

    {"struct_get_full_name",
     (PyCFunction)struct_get_full_name,
     METH_VARARGS,
     "Get the full name of the struct"},

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

  return PyModule_Create(&module_def);
}
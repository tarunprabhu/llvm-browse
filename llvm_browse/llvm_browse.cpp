#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "lib/Function.h"
#include "lib/GlobalAlias.h"
#include "lib/GlobalVariable.h"
#include "lib/MDNode.h"
#include "lib/Module.h"
#include "lib/StructType.h"

#include <type_traits>

using Handle = uint64_t;

// The tags are added to the Handle to be able to determine the dynamic type
// of the object from the handle
enum class HandleTag {
  Module   = 0x0,
  Alias    = 0x1,
  Function = 0x2,
  Global   = 0x3,
  Struct   = 0x4,
  Metadata = 0x5,
  Mask     = 0xf,
};

static const Handle HandleInvalid = 0;
static const char* HandleFmt = "k";

template<typename T, std::enable_if_t<!std::is_pointer<T>::value, int> = 0>
static Handle
get_handle(const T* ptr, HandleTag tag) {
  return reinterpret_cast<Handle>(ptr) | static_cast<Handle>(tag);
}

template<typename T, std::enable_if_t<!std::is_pointer<T>::value, int> = 0>
static PyObject*
get_py_handle(const T* ptr, HandleTag tag) {
	return PyLong_FromLong(get_handle(ptr, tag));
}

template<typename T>
static T*
get_object(Handle handle) {
  return reinterpret_cast<T*>(handle & ~static_cast<Handle>(HandleTag::Mask));
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
  return get_py_handle(lb::Module::create(file).release(), HandleTag::Module);
}

static PyObject*
module_get_aliases(PyObject* self, PyObject* args) {
  Handle handle = HandleInvalid;

  if(!PyArg_ParseTuple(args, HandleFmt, &handle))
    return nullptr;

  const lb::Module* module = get_object<lb::Module>(handle);
  PyObject* list           = PyList_New(0);
  for(const lb::GlobalAlias* alias : module->aliases())
    PyList_Append(list, get_py_handle(alias, HandleTag::Alias));
  return list;
}

static PyObject*
module_get_functions(PyObject* self, PyObject* args) {
	Handle handle = HandleInvalid;
	if(!PyArg_ParseTuple(args, HandleFmt, &handle))
		return nullptr;

	const lb::Module* module = get_object<lb::Module>(handle);
	PyObject* list = PyList_New(0);
	for(const lb::Function* f : module->functions())
		PyList_Append(list, get_py_handle(f, HandleTag::Function));
	return list;
}

static PyObject*
module_get_globals(PyObject* self, PyObject* args) {
	Handle handle = HandleInvalid;
	if(!PyArg_ParseTuple(args, HandleFmt, &handle))
		return nullptr;

	const lb::Module* module = get_object<lb::Module>(handle);
	PyObject* list = PyList_New(0);
	for(const lb::GlobalVariable* g : module->globals())
		PyList_Append(list, get_py_handle(g, HandleTag::Function));
	return list;
}

static PyObject*
module_get_structs(PyObject* self, PyObject* args) {
	Handle handle = HandleInvalid;
	if(!PyArg_ParseTuple(args, HandleFmt, &handle))
		return nullptr;

	const lb::Module* module = get_object<lb::Module>(handle);
	PyObject* list = PyList_New(0);
	for(const lb::StructType* s : module->structs())
		PyList_Append(list, get_py_handle(s, HandleTag::Struct));
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
entity_get_source_name(PyObject* self, PyObject* args) {
  Handle handle = HandleInvalid;
  if(!PyArg_ParseTuple(args, HandleFmt, &handle))
    return nullptr;
}

static PyObject*
entity_get_llvm_name(PyObject* self, PyObject* args) {
  Handle handle = HandleInvalid;
  if(!PyArg_ParseTuple(args, HandleFmt, &handle))
    return nullptr;
}

static PyObject*
entity_get_uses(PyObject* self, PyObject* args) {
  Handle handle = HandleInvalid;
  if(!PyArg_ParseTuple(args, HandleFmt, &handle))
    return nullptr;
}

static PyMethodDef module_methods[]
    = {{"module_create",
        (PyCFunction)module_create,
        METH_VARARGS,
        "Create a new module and return a handle to it"},
       {"module_free",
        (PyCFunction)module_free,
        METH_VARARGS,
        "Frees a module created by module_create"},
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
       {"entity_get_source_name",
        (PyCFunction)entity_get_source_name,
        METH_VARARGS,
        "Gets the source name of the entity"},
       {"entity_get_llvm_name",
        (PyCFunction)entity_get_llvm_name,
        METH_VARARGS,
        "Gets the LLVM name of the entity. This could be a slot"},
       {NULL, NULL, 0, NULL}};

static struct PyModuleDef module_def = {
    PyModuleDef_HEAD_INIT,
    "llvm_browse",
    "Python module to interface with for libLLVMBrowse",
    -1,
    module_methods,
    NULL,
    NULL,
    NULL,
    NULL,
};

extern "C" PyMODINIT_FUNC
PyInit_llvm_browse(void) {
  PyObject* module = PyModule_Create(&module_def);

  return module;
}
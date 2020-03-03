#include "lib/LLVMBrowse.h"
#include "lib/Module.h"
#include "lib/GlobalAlias.h"
#include "lib/Function.h"
#include "lib/GlobalVariable.h"
#include "lib/StructType.h"

template<typename T, std::enable_if_t<!std::is_pointer<T>::value, int> = 0>
static LB_Handle
get_handle(const T* ptr) {
  return reinterpret_cast<LB_Handle>(ptr);
}

template<typename T, std::enable_if_t<!std::is_pointer<T>::value, int> = 0>
static LB_Handle
get_handle(const T& ref) {
	return get_handle(&ref);
}

template<typename T>
static T* get_object(LB_Handle handle) {
	return reinterpret_cast<T*>(handle);
}

extern "C" void
lb_list_handle_free(LB_List_Handle handles) {
	delete handles;
}

extern "C" LB_Handle
lb_module_create(const char* path) {
	// Module::create returns a std::unique_ptr. We don't want the caller to 
	// own this, so we just release it from the returned pointer and hand
	// the pointer off to the caller. It is the caller's responsibilty to 
	// call lb_module_free() to release the Module 
	return get_handle(lb::Module::create(path).release());
}

extern "C" void
lb_module_free(LB_Handle handle) {
  delete get_object<lb::Module>(handle);
}

extern "C" unsigned
lb_module_get_num_aliases(LB_Handle handle) {
	return get_object<lb::Module>(handle)->get_num_aliases();
}

extern "C" LB_List_Handle
lb_module_populate_aliases(LB_Handle handle, LB_List_Handle aliases) {
	const lb::Module* module = get_object<lb::Module>(handle);
	unsigned i = 0;
	for(const lb::GlobalAlias* alias : module->aliases())
		aliases[i++] = get_handle(alias);
	return aliases;
}

extern "C" unsigned
lb_module_get_num_functions(LB_Handle handle) {
	return get_object<lb::Module>(handle)->get_num_functions();
}

extern "C" LB_List_Handle
lb_module_populate_functions(LB_Handle handle, LB_List_Handle functions) {
	const lb::Module* module = get_object<lb::Module>(handle);
	unsigned i = 0;
	for(const lb::Function* f : module->functions())
		functions[i++] = get_handle(f);
	return functions;
}

extern "C" unsigned 
lb_module_get_num_globals(LB_Handle handle) {
	return get_object<lb::Module>(handle)->get_num_globals();
}

extern "C" LB_List_Handle
lb_module_populate_globals(LB_Handle handle, LB_List_Handle globals) {
  const lb::Module* module = get_object<lb::Module>(handle);
  unsigned i               = 0;
  for(const lb::GlobalVariable* g : module->globals())
    globals[i++] = get_handle(g);
  return globals;
}

extern "C" unsigned
lb_module_get_num_structs(LB_Handle handle) {
	return get_object<lb::Module>(handle)->get_num_structs();
}

extern "C" LB_List_Handle
lb_module_populate_structs(LB_Handle handle, LB_List_Handle structs) {
	const lb::Module* module = get_object<lb::Module>(handle);
	unsigned i = 0;
	for(const lb::StructType* s : module->structs())
		structs[i++] = get_handle(s);
	return structs;
}

extern "C" const char*
lb_module_get_llvm(LB_Handle handle) {
	return get_object<lb::Module>(handle)->get_contents_as_cstr();
}
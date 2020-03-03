#ifndef LLVM_BROWSE_LLVM_BROWSE_H
#define LLVM_BROWSE_LLVM_BROWSE_H

#include <stdint.h>

typedef uint64_t LB_Handle;
typedef LB_Handle* LB_List_Handle;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

struct CSourceLoc {
	const char* file;
	unsigned begin_line;
	unsigned begin_col;
	unsigned end_line;
	unsigned end_col;
};

struct CLLVMLoc {
	uint64_t begin;
	uint64_t end;
};

void
lb_list_handle_free(LB_List_Handle list);

LB_Handle
lb_module_create(const char* file);

unsigned
lb_module_get_num_aliases(LB_Handle module);
unsigned
lb_module_get_num_functions(LB_Handle module);
unsigned
lb_module_get_num_globals(LB_Handle module);
unsigned
lb_module_get_num_structs(LB_Handle module);

// The aliases array is assumed to be large enough to contain all the
// aliases. It should typically be allocated by calling
// lb_module_get_num_aliases()
LB_List_Handle
lb_module_populate_aliases(LB_Handle module, LB_List_Handle aliases);
LB_List_Handle
lb_module_populate_functions(LB_Handle module, LB_List_Handle functions);
LB_List_Handle
lb_module_populate_globals(LB_Handle module, LB_List_Handle globals);
LB_List_Handle
lb_module_populate_structs(LB_Handle module, LB_List_Handle structs);

const char* lb_module_get_llvm(LB_Handle module);

void lb_module_free(LB_Handle);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // LLVM_BROWSE_LLVM_BROWSE_H
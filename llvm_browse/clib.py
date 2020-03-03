#!/usr/bin/env python3

from .types import SourceRange, SourceLoc, LLVMLoc

from ctypes import cdll
from ctypes import c_char_p, c_bool, c_uint, c_ulong, Structure
from ctypes import POINTER as cptr
from typing import List
import os
import sys

clib = None
if sys.platform == 'linux':
    clib = cdll.LoadLibrary(os.path.join(os.path.dirname(__file__),
                                         'libLLVMBrowse.so'))
elif sys.platform == 'darwin':
    clib = cdll.LoadLibrary(os.path.join(os.path.dirname(__file__),
                                         'libLLVMBrowse.so'))
elif sys.platform == 'win32':
    clib = cdll.LoadLibrary(os.path.join(os.path.dirname(__file__),
                                         'LLVMBrowse.dll'))


class CSourceLoc(Structure):
    _fields_ = [
        ('file', c_char_p),
        ('begin_line', c_uint),
        ('begin_column', c_uint),
        ('end_line', c_uint),
        ('end_column', c_uint),
    ]


class CLLVMLoc(Structure):
    _fields_ = [
        ('begin', c_ulong),
        ('end', c_ulong),
    ]


lb_handle_t = c_ulong

# Returns a types representing a C array


def lb_list_handle_t(count: int):
    return lb_handle_t * count


def lb_entity_get_source_loc(handle: int) -> SourceLoc:
    f_has_source = clib.lb_entity_has_source_loc
    f_has_source.argtypes = [lb_handle_t]
    f_has_source.restype = c_bool
    if not f_has_source(handle):
        return None

    f = clib.lb_entity_populate_source_loc
    f.argtypes = [lb_handle_t, cptr(CSourceLoc)]
    f.restype = c_bool

    loc = CSourceLoc()
    f(handle, loc)
    return SourceLoc(loc.file,
                     SourceRange(loc.begin_line, loc.begin_column),
                     SourceRange(loc.end_line, loc.end_column))


def lb_entity_get_llvm_loc(handle: int) -> LLVMLoc:
    f = clib.lb_entity_populate_llvm_loc
    f.argtypes = [lb_handle_t, cptr(CLLVMLoc)]
    f.restype = c_bool

    loc = CLLVMLoc()
    f(handle, loc)
    return LLVMLoc(loc.begin, loc.end)


def lb_module_create(file: str) -> int:
    f = clib.lb_module_create
    f.argtypes = [c_char_p]
    f.restype = lb_handle_t

    return f(file.encode('utf-8'))


def _lb_module_get_entities(module: int, fn_num, fn_populate) -> List[int]:
    fn_num.argtypes = [lb_handle_t]
    fn_num.restype = c_uint
    count = fn_num(module)
    if not count:
        return []

    fn_populate.argtypes = [lb_handle_t, lb_list_handle_t(count)]
    fn_populate.restype = lb_list_handle_t(count)

    inp = [0] * count
    cinp = lb_list_handle_t(count)(*inp)
    fn_populate(module, cinp)

    return list(cinp)


def lb_module_get_aliases(module: int) -> List[int]:
    return _lb_module_get_entities(module,
                                   clib.lb_module_get_num_aliases,
                                   clib.lb_module_populate_aliases)


def lb_module_get_functions(module: int) -> List[int]:
    return _lb_module_get_entities(module,
                                   clib.lb_module_get_num_functions,
                                   clib.lb_module_populate_functions)


def lb_module_get_globals(module: int) -> List[int]:
    return _lb_module_get_entities(module,
                                   clib.lb_module_get_num_globals,
                                   clib.lb_module_populate_globals)


def lb_module_get_structs(module: int) -> List[int]:
    return _lb_module_get_entities(module,
                                   clib.lb_module_get_num_structs,
                                   clib.lb_module_populate_structs)


def lb_module_get_llvm(handle: int) -> str:
    f = clib.lb_module_get_llvm
    f.argtypes = [lb_handle_t]
    f.restype = c_char_p

    print('handle', handle)
    return f(handle).decode('utf-8')


def lb_module_free(handle: int):
    f = clib.lb_module_free
    f.argtypes = [lb_handle_t]
    f(handle)

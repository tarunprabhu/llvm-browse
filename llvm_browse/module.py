#!/usr/bin/env python3

from .clib import lb_module_create
from .clib import (lb_module_get_aliases,
                   lb_module_get_functions,
                   lb_module_get_globals,
                   lb_module_get_structs)
from typing import List


class Module:
    def __init__(self, handle: int):
        self.handle = handle
        self.aliases = lb_module_get_aliases(self.handle)
        self.functions = lb_module_get_functions(self.handle)
        self.globals = lb_module_get_globals(self.handle)
        self.structs = lb_module_get_structs(self.handle)

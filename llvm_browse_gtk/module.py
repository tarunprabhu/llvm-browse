#!/usr/bin/env python3

import llvm_browse as lb

from .alias import Alias
from .function import Function
from .global_variable import Global
from .metadata import Metadata
from .struct import Struct
from typing import List

import gi
gi.require_version('GObject', '${PY_GOBJECT_VERSION}')
from gi.repository import GObject # NOQA: E402


class Module(GObject.GObject):
    handle = GObject.Property(
        type=GObject.TYPE_UINT64,
        default=0,
        nick='handle',
        blurb='Handle needed by llvm_browse')

    def __init__(self, handle: int):
        GObject.GObject.__init__(self)

        self.handle = handle

        self.aliases = [Alias(h) for h in lb.module_get_aliases(self.handle)]
        self.functions = [Function(h) for h in lb.module_get_functions(self.handle)]
        self.globals = [Global(h) for h in lb.module_get_globals(self.handle)]
        self.structs = [Struct(h) for h in lb.module_get_structs(self.handle)]

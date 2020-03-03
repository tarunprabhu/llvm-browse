#!/usr/bin/env python3

import argparse
from .clib import lb_module_create, lb_module_free
from .module import Module
from .ui import UI
from .options import Options
from typing import Union, List
import gi
gi.require_version('GLib', '2.0')
gi.require_version('GObject', '2.0')
gi.require_version('Gtk', '3.0')
gi.require_version('GtkSource', '4')
from gi.repository import GLib, GObject, Gtk, GtkSource  # NOQA: E402

# Not sure why I need to register the GtkSourceView, but best to do it
# here so we can be sure that it gets registered before anything else
GObject.type_register(GtkSource.View)


class Application(Gtk.Application):
    llvm = GObject.Property(
        type=GObject.TYPE_UINT64,
        default=0,
        nick='llvm',
        blurb='The path to the LLVM file currently shown or ""')

    llvm_file = GObject.Property(
        type=str,
        default='',
        nick='llvm-file',
        blurb='The LLVM file that is currently loaded')

    # This property is here because we cannot bind the LLVM string property
    # directly to boolean properties of widgets because we can't set
    # transform functions when binding properties
    loaded = GObject.Property(
        type=bool,
        default=False,
        nick='loaded',
        blurb='True if a LLVM file has been loaded')

    def __init__(self):
        Gtk.Application.__init__(self)

        GLib.set_application_name('LLVM Browse')
        GLib.set_prgname('llvm-browse')

        self.argv: argparse.Namespace = None
        self.options: Options = Options(self)
        self.ui: UI = UI(self)
        self.module: Module = None

    def _reset(self):
        self.llvm = 0
        self.llvm_file = ''

    def do_activate(self) -> bool:
        self.options.load()
        self.add_window(self.ui.get_application_window())
        self.ui.emit('launch')
        if self.argv.file:
            self.action_open(self.argv.file)
        return False

    # Returns true if the file could be opened
    def action_open(self, file: str) -> bool:
        self.llvm_file = file
        self.llvm = lb_module_create(file)
        self.module = Module(self.llvm)
        if not self.llvm:
            self._reset()
        else:
            self.ui.do_open()
        return bool(self.llvm)

    # Returns true if the file could be closed
    def action_close(self):
        if self.llvm:
            lb_module_free(self.llvm)
        self._reset()
        return True

    # Returns true if the file could be reloaded
    def action_reload(self):
        if self.llvm_file:
            if not self.action_open(self, self.llvm_file):
                self._reset()
                return False
            return True
        return False

    # Returns true on success. Not sure if this will actually return
    def action_quit(self) -> bool:
        if self.llvm:
            self.lb_module_free(self.llvm)
            self._reset()
        self.remove_window(self.ui.get_application_window())
        return True

    def action_goto_definition(self):
        pass

    def action_goto_prev_use(self):
        pass

    def action_goto_next_use(self):
        pass

    def action_go_back(self):
        pass

    def action_go_forward(self):
        pass

    def run(self, argv: argparse.Namespace) -> int:
        self.argv = argv
        ret = Gtk.Application.run(self)
        self.options.store()
        return ret

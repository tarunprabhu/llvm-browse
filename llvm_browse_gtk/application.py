#!/usr/bin/env python3

import argparse
import llvm_browse as lb
from .module import Module
from .options import Options
from typing import Union, List
from .ui import UI
import gi
gi.require_version('GLib', '${PY_GLIB_VERSION}')
gi.require_version('GObject', '${PY_GOBJECT_VERSION}')
gi.require_version('Gtk', '${PY_GTK_VERSION}')
gi.require_version('GtkSource', '${PY_GTKSOURCE_VERSION}')
from gi.repository import GLib, GObject, Gtk, GtkSource  # NOQA: E402

# Not sure why I need to register the GtkSourceView, but best to do it
# here so we can be sure that it gets registered before anything else
GObject.type_register(GtkSource.View)


class Application(Gtk.Application):
    handle = GObject.Property(
        type=GObject.TYPE_UINT64,
        default=0,
        nick='handle',
        blurb='Handle to the LLVM module')

    module = GObject.Property(
        type=GObject.GObject,
        default=None,
        nick='module',
        blurb='A module object wrapper')

    llvm = GObject.Property(
        type=str,
        default='',
        nick='llvm',
        blurb='The path to the LLVM file currently shown or ""')

    def __init__(self):
        Gtk.Application.__init__(self)

        GLib.set_application_name('LLVM Browse')
        GLib.set_prgname('llvm-browse')

        self.argv: argparse.Namespace = None
        self.options: Options = Options(self)
        self.ui: UI = UI(self)

    def _reset(self):
        self.handle = 0
        self.llvm = ''
        self.module = None

    def do_activate(self) -> bool:
        self.options.load()
        self.add_window(self.ui.get_application_window())
        self.ui.emit('launch')
        if self.argv.maximize:
            self.ui.win_main.maximize()
        if self.argv.file:
            self.action_open(self.argv.file)
        return False

    # Returns true if the file could be opened
    def action_open(self, file: str) -> bool:
        self.llvm = file
        self.handle = lb.module_create(file)
        if not self.handle:
            self._reset()
        else:
            self.module = Module(self.handle)
            self.ui.do_open()
        return bool(self.handle)

    # Returns true if the file could be closed
    def action_close(self):
        if self.handle:
            lb_module_free(self.handle)
        self._reset()
        return True

    # Returns true if the file could be reloaded
    def action_reload(self):
        if self.llvm:
            if not self.action_open(self, self.llvm):
                self._reset()
                return False
            return True
        return False

    # Returns true on success. Not sure if this will actually return
    def action_quit(self) -> bool:
        if self.handle:
            self.lb_module_free(self.handle)
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

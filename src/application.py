#!/usr/bin/env python3

from typing import Union, List
import gi
gi.require_version('GLib', '2.0')
gi.require_version('GObject', '2.0')
gi.require_version('Gtk', '3.0')
gi.require_version('GtkSource', '4')
from gi.repository import GLib, GObject, Gtk, GtkSource # NOQA: E402

from options import Options
from ui import UI

class LLVMBrowse(Gtk.Application):
    llvm = GObject.Property(
        type=str,
        default='',
        nick='llvm',
        blurb='The path to the LLVM file currently shown or ""')

    # This property is here because we cannot bind the LLVM string property
    # directly to boolean properties of widgets because we can't set 
    # transform functions when binding properties
    loaded = GObject.Property(
        type=bool,
        default=False,
        nick='opened',
        blurb='True if a LLVM file has been loaded')

    len = GObject.Property(type=int, default=0)

    def __init__(self):
        Gtk.Application.__init__(self)

        GLib.set_application_name('LLVM Browse')
        GLib.set_prgname('llvm-browse')

        self.options: Options = Options(self)
        self.ui: UI = UI(self)

    def do_activate(self) -> bool:
        self.options.load()
        self.add_window(self.ui.get_application_window())
        self.ui.emit('launch')
        return False

    def action_open(self):
        pass

    def action_close(self):
        pass

    def action_reload(self):
        pass

    def action_quit(self):
        self.remove_window(self.ui.get_application_window())
        pass

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

    def run(self, argv: List[str]) -> int:
        ret = Gtk.Application.run(self)
        self.options.store()
        return ret

#!/usr/bin/env python3

import gi
gi.require_version('Gtk', '3.0')
gi.require_version('GObject', '2.0')
gi.require_version('GtkSource', '4')
from gi.repository import Gtk, GObject, GtkSource  # NOQA: E402


class UI(GObject.GObject):
    def __init__(self, app):
        GObject.GObject.__init__(self)

        # FIXME: Read the glade UI description from a resource file
        self.builder = Gtk.Builder.new_from_file(
            '/home/tarun/code/llvm-browse/resources/ui/llvm-browse.glade')

        self.app = app
        self.options: Options = self.app.options

        self._bind_options()
        self._bind_widget_properties()
        self._bind_state_properties()

        self.builder.connect_signals(self)

    def __getitem__(self, name: str) -> GObject.GObject:
        return self.builder.get_object(name)

    def _bind(self,
              src: GObject,
              src_prop: str,
              dst: GObject.GObject,
              dst_prop: str,
              bidirectional: bool = True,
              invert: bool = False) -> GObject.Binding:
        flags = GObject.BindingFlags.SYNC_CREATE
        if bidirectional:
            flags |= GObject.BindingFlags.BIDIRECTIONAL
        if invert:
            flags |= GObject.BindingFlags.INVERT_BOOLEAN
        return src.bind_property(src_prop, dst, dst_prop, flags)

    def _bind_options(self):
        def bind(src_prop: str,
                 dst: GObject.GObject,
                 dst_prop: GObject.GObject,
                 bidirectional: bool = True,
                 invert: bool = False):
            return self._bind(self.options, src_prop, dst,
                              dst_prop, bidirectional, invert)

        bind('font', self['fbtn_options_code'], 'font-desc')
        bind('style', self['srcvw_llvm'].get_buffer(), 'style-scheme')
        bind('style', self['srcstyl_options_code'], 'style-scheme')
        bind('line-nums-llvm', self['srcvw_llvm'], 'show-line-numbers')
        bind('line-nums-llvm', self['chk_options_lines_llvm'], 'active')
        bind('line-nums-source', self['srcvw_source'], 'show-line-numbers')
        bind('line-nums-source', self['chk_options_lines_source'], 'active')
        bind('window-maximized', self['swch_options_maximized'], 'active')

        # show-contents and show-source only apply when the application is
        # first started. In any session, even if the user chooses to show/hide
        # those panels, the configuration setting should not be affected
        bind('show-contents', self['grd_contents'],
             'visible', bidirectional=False)
        bind('show-contents', self['swch_options_contents'], 'active')
        bind('show-source', self['grd_source'], 'visible', bidirectional=False)
        bind('show-source', self['swch_options_source'], 'active')

    def _bind_widget_properties(self):
        self._bind(self['mitm_view_contents'], 'active',
                   self['grd_contents'], 'visible')
        self._bind(self['mitm_view_source'], 'active',
                   self['grd_source'], 'visible')
        self._bind(self['mitm_view_toolbar'], 'active',
                   self['tlbar_main'], 'visible')

    def _bind_state_properties(self):
        def bind(src_prop: str,
                 dst: GObject.GObject,
                 dst_prop: GObject.GObject,
                 bidirectional: bool = False,
                 invert: bool = False):
            return self._bind(self.app, src_prop, dst,
                              dst_prop, bidirectional, invert)

        for widget in (self['mitm_reload'],
                       self['tlbtn_reload'],
                       self['mitm_close'],
                       self['tlbtn_close'],
                       self['mitm_search_forward'],
                       self['mitm_search_backward'],
                       self['mitm_goto_definition'],
                       self['tlbtn_goto_definition'],
                       self['mitm_goto_prev_use'],
                       self['tlbtn_goto_prev_use'],
                       self['mitm_goto_next_use'],
                       self['tlbtn_goto_next_use'],
                       self['mitm_go_back'],
                       self['tlbtn_go_back'],
                       self['mitm_go_forward'],
                       self['tlbtn_go_back']):
            bind('loaded', widget, 'sensitive')

    @GObject.Signal
    def launch(self, *args):
        self['win_main'].show()
        if self.options.window_maximized:
            self['win_main'].maximize()

    def on_open(self, *args) -> bool:
        dlg = self['dlg_file']
        response = dlg.run()
        if response == Gtk.ResponseType.RESPONSE_OK:
            pass
        dlg.hide()
        return True

    def on_close(self, *args) -> bool:
        return True

    def on_reload(self, *args) -> bool:
        return False

    def on_quit(self, *args) -> bool:
        self['win_main'].hide()
        self.app.action_quit()
        return False

    def on_options(self, *args) -> bool:
        dlg = self['dlg_options']
        response = dlg.run()
        if response == Gtk.ResponseType.APPLY:
            self.options.store()
        dlg.hide()
        return False

    def on_about(self, *args) -> bool:
        dlg = self['dlg_about']
        dlg.run()
        dlg.hide()
        return True

    def on_goto_line(self, *args) -> bool:
        return False

    def on_goto_definition(self, *args) -> bool:
        return False

    def on_goto_prev_use(self, *args) -> bool:
        return False

    def on_goto_next_use(self, *args) -> bool:
        return False

    def on_go_back(self, *args) -> bool:
        return False

    def on_go_forward(self, *args) -> bool:
        return False

    def on_show_source(self, *args) -> bool:
        return False

    def on_search_forward(self, *args) -> bool:
        return False

    def on_search_backward(self, *args) -> bool:
        return False

    def on_toggle_fullscreen(self, *args) -> bool:
        return False

    def get_application_window(self) -> Gtk.ApplicationWindow:
        return self['win_main']

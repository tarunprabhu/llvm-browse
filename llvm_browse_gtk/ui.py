#!/usr/bin/env python3

from .entity import Entity
from enum import IntEnum
import llvm_browse as lb
import os
import gi
gi.require_version('GObject', '${PY_GOBJECT_VERSION}')
gi.require_version('GLib', '${PY_GLIB_VERSION}')
gi.require_version('Gdk', '${PY_GTK_VERSION}')
gi.require_version('GdkPixbuf', '${PY_GDKPIXBUF_VERSION}')
gi.require_version('Gio', '${PY_GIO_VERSION}')
gi.require_version('Gtk', '${PY_GDK_VERSION}')
gi.require_version('Pango', '${PY_PANGO_VERSION}')
gi.require_version('GtkSource', '${PY_GTKSOURCE_VERSION}')
from gi.repository import (GObject, GLib, Gdk, GdkPixbuf,
                           Gio, Gtk, GtkSource, Pango)  # NOQA: E402


class ModelColsContents(IntEnum):
    Entity = 0
    Display = 1
    Tooltip = 2
    Style = 3


class UI(GObject.GObject):
    def __init__(self, app):
        GObject.GObject.__init__(self)

        Gio.Resource.load(os.path.join(os.path.dirname(__file__),
                                       'llvm-browse.gresource'))._register()

        self.builder = Gtk.Builder.new_from_resource(
            '/llvm-browse/gtk/llvm-browse.ui')
        self.srcvw_llvm = self['srcvw_llvm']
        self.srcbuf_llvm = self.srcvw_llvm.get_buffer()
        self.srcvw_code = self['srcvw_source']
        self.srcbuf_code = self.srcvw_code.get_buffer()
        self.mgr_lang = GtkSource.LanguageManager.get_default()
        self.mgr_style = GtkSource.StyleSchemeManager.get_default()
        self.win_main = self['win_main']
        self.pnd_body = self['pnd_body']
        self.pnd_code = self['pnd_code']

        icon16 = GdkPixbuf.Pixbuf.new_from_resource(
            '/llvm-browse/icons/16x16/llvm-browse.svg')
        icon64 = GdkPixbuf.Pixbuf.new_from_resource(
            '/llvm-browse/icons/64x64/llvm-browse.svg')

        self['win_main'].set_icon(icon16)
        self['dlg_file'].set_icon(icon16)
        self['dlg_options'].set_icon(icon16)
        self['dlg_about'].set_icon(icon16)
        self['dlg_about'].set_logo(icon64)

        self.app = app
        self.options: Options = self.app.options

        self._init_widgets()
        self._bind_options()
        self._bind_widget_properties()
        self._bind_state_properties()

        self.builder.connect_signals(self)

    def __getitem__(self, name: str) -> GObject.GObject:
        return self.builder.get_object(name)

    def _init_widgets(self):
        self.srcbuf_llvm.set_language(self.mgr_lang.get_language('llvm'))

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
                       self['mitm_goto_line'],
                       self['mitm_goto_definition'],
                       self['tlbtn_goto_definition'],
                       self['mitm_goto_prev_use'],
                       self['tlbtn_goto_prev_use'],
                       self['mitm_goto_next_use'],
                       self['tlbtn_goto_next_use'],
                       self['mitm_go_back'],
                       self['tlbtn_go_back'],
                       self['mitm_go_forward'],
                       self['tlbtn_go_forward']):
            bind('handle', widget, 'sensitive')

    @GObject.Signal
    def launch(self, *args):
        self['win_main'].show()
        if self.options.window_maximized:
            self['win_main'].maximize()

    def do_async(self, fn, *args):
        def impl(*args):
            fn(*args)
            return False

        GLib.idle_add(impl, *args, priority=GLib.PRIORITY_DEFAULT_IDLE)

    def do_open(self):
        def get_style(entity: Entity) -> Pango.Style:
            if entity.artificial:
                return Pango.Style.ITALIC
            return Pango.Style.NORMAL

        def get_tooltip(entity: Entity) -> str:
            if (not entity.source_name) and (not entity.full_name):
                return ''

            # TODO: Use the same font as the rest of the code
            out = []
            out.append('<tt>')
            if entity.source_name:
                out.append('<b>{:7}</b> {}'.format(
                    'Source',
                    GLib.markup_escape_text(entity.source_name)))
            if entity.full_name:
                # We will never have the situation where an entity has a full
                # name but no source name because the full name will have 
                # been computed from the source name. So it's safe to prepend
                # the string with a newline
                out.append('\n<b>{:7}</b> {}'.format(
                    'Full',
                    GLib.markup_escape_text(entity.full_name)))
            out.append('</tt>')

            return ''.join(out)

        module: Module = self.app.module
        self.srcbuf_llvm.set_text(lb.module_get_code(module.handle))

        trvw = self['trvw_contents']
        trfltr = trvw.get_model()
        trst = self['trst_contents']

        trvw.set_model(None)
        trst.clear()
        for label, entities in [('Aliases', module.aliases),
                                ('Functions', module.functions),
                                ('Globals', module.globals),
                                ('Structs', module.structs)]:
            trit = trst.append(None, [None, label, '', Pango.Style.NORMAL])
            for entity in entities:
                trst.append(trit,
                            [entity,
                             entity.llvm_name,
                             get_tooltip(entity),
                             get_style(entity)])
        trvw.set_model(trfltr)

    def on_open_recent(self, widget: Gtk.Widget) -> bool:
        file, _ = GLib.filename_from_uri(widget.get_current_item().get_uri())
        if file:
            self.app.action_open(file)
        return False

    def on_open(self, widget: Gtk.Widget) -> bool:
        dlg = self['dlg_file']
        response = dlg.run()
        if response == Gtk.ResponseType.OK:
            # FIXME: Do this in a separate thread instead of blocking the
            # UI by doing this here
            self.app.action_open(dlg.get_filename())
        dlg.hide()
        return False

    def on_close(self, *args) -> bool:
        self.app.action_close()
        self.srcbuf_llvm.set_text('')
        return False

    def on_reload(self, *args) -> bool:
        # FIXME: Do this in a separate thread instead of blocking the UI
        # by doing this
        if self.app.action_reload():
            self.do_open()
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
        return False

    def on_entity_selected(self,
                           trvw: Gtk.TreeView,
                           path: Gtk.TreePath,
                           col: Gtk.TreeViewColumn) -> bool:
        def update_ui(i: Gtk.TreeIter) -> bool:
            self.srcvw_llvm.scroll_to_iter(i, 0.1, True, 0, 0)
            return False

        model = trvw.get_model()
        row = model.get_iter(path)
        entity = model[row][ModelColsContents.Entity]
        if entity:
            offset = entity.llvm_defn.begin
            off_iter = self.srcbuf_llvm.get_iter_at_offset(offset)
            line = off_iter.get_line()
            line_iter = self.srcbuf_llvm.get_iter_at_line(line)
            self.srcbuf_llvm.place_cursor(line_iter)
            self.do_async(update_ui, line_iter)

        return False

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
        if self['mitm_toggle_fullscreen'].get_active():
            self.win_main.fullscreen()
        else:
            self.win_main.unfullscreen()
        return False

    def get_application_window(self) -> Gtk.ApplicationWindow:
        return self.win_main

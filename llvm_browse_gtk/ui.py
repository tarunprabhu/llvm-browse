#!/usr/bin/env python3

from enum import auto, Enum, IntEnum
from typing import Tuple, List, Mapping
import llvm_browse as lb
import operator
import os
import re
import gi
gi.require_version('GObject', '${PY_GOBJECT_VERSION}')
gi.require_version('GLib', '${PY_GLIB_VERSION}')
gi.require_version('Gdk', '${PY_GTK_VERSION}')
gi.require_version('GdkPixbuf', '${PY_GDKPIXBUF_VERSION}')
gi.require_version('Gio', '${PY_GIO_VERSION}')
gi.require_version('Gtk', '${PY_GDK_VERSION}')
gi.require_version('Pango', '${PY_PANGO_VERSION}')
gi.require_version('PangoCairo', '${PY_PANGOCAIRO_VERSION}')
gi.require_version('GtkSource', '${PY_GTKSOURCE_VERSION}')
from gi.repository import (GObject, GLib, Gdk, GdkPixbuf,
                           Gio, Gtk, GtkSource, Pango, PangoCairo)  # NOQA: E402


class ModelColsContents(IntEnum):
    Entity = 0
    Display = 1
    Tooltip = 2
    Style = 3
    Weight = 4
    Font = 5


class SearchDirection(Enum):
    Forward = auto()
    Backward = auto()


class SearchState:
    def __init__(self, direction: SearchDirection, i: Gtk.TextIter):
        self.direction = direction
        self.iter_start = i
        self.iter_curr = i
        self.flags = Gtk.TextSearchFlags.TEXT_ONLY \
            | Gtk.TextSearchFlags.CASE_INSENSITIVE


class UI(GObject.GObject):
    @staticmethod
    def fn_font_filter(family, face) -> bool:
        # If the SelectionLevel in Glade doesn't include STYLE, then some
        # font variants are missing in the FontChooser widget. For example,
        # you see Liberation Mono Bold, but not Liberation Mono Regular.
        # Not really sure what's up with that but including the styles
        # gets you everything including *all* the variants. So we now have
        # to filter out all the non-regular stuff. Quite silly, really!
        return face.get_face_name() == 'Regular'

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
        self.tag_table = self.srcbuf_llvm.get_tag_table()
        self.mgr_lang = GtkSource.LanguageManager.get_default()
        self.mgr_style = GtkSource.StyleSchemeManager.get_default()
        self.win_main = self['win_main']
        self.pnd_body = self['pnd_body']
        self.pnd_code = self['pnd_code']
        self.trvw_contents = self['trvw_contents']
        self.trst_contents = self['trst_contents']
        self.trfltr_contents = self['trfltr_contents']
        self.trsrt_contents = self['trsrt_contents']

        icon16 = GdkPixbuf.Pixbuf.new_from_resource(
            '/llvm-browse/icons/16x16/llvm-browse.svg')
        icon64 = GdkPixbuf.Pixbuf.new_from_resource(
            '/llvm-browse/icons/64x64/llvm-browse.svg')

        self['win_main'].set_icon(icon16)
        self['dlg_file'].set_icon(icon16)
        self['dlg_options'].set_icon(icon16)
        self['dlg_about'].set_icon(icon16)
        self['dlg_about'].set_logo(icon64)

        self.lang_mgr = GtkSource.LanguageManager.get_default()
        self.app = app
        self.options: Options = self.app.options
        self.search = None

        self.srcbuf_llvm.connect(
            'notify::cursor-position', self.on_cursor_moved)

        self._init_widgets()
        self._bind_options()
        self._bind_widget_properties()
        self._bind_state_properties()

        self.builder.connect_signals(self)

    def __getitem__(self, name: str) -> GObject.GObject:
        return self.builder.get_object(name)

    def _init_widgets(self):
        self.srcbuf_llvm.set_language(self.lang_mgr.get_language('llvm'))
        self.trsrt_contents.set_sort_func(1, self.fn_contents_sort_names)
        self.trfltr_contents.set_visible_func(self.fn_contents_filter_names)
        self['fbtn_options_code'].set_filter_func(UI.fn_font_filter)

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

        # FIXME: The GtkSource.View because unusable for large LLVM files
        # (100MB+). At some point, it would be nice to switch to a custom text
        # view or do some pre-processing so that we can keep the syntax
        # highlighting even for the really large files
        bind('font', self['fbtn_options_code'], 'font-desc')
        bind('style', self['srcstyl_options_code'], 'style-scheme')
        bind('syntax-llvm', self['swch_options_syntax_llvm'], 'active')
        bind('style-llvm', self.srcbuf_llvm, 'style-scheme')
        bind('syntax-source', self['swch_options_syntax_source'], 'active')
        bind('style-source', self.srcbuf_code, 'style-scheme')
        bind('line-nums-llvm', self['srcvw_llvm'], 'show-line-numbers')
        bind('line-nums-llvm', self['swch_options_linenums_llvm'], 'active')
        bind('line-nums-source', self['srcvw_source'], 'show-line-numbers')
        bind('line-nums-source',
             self['swch_options_linenums_source'], 'active')
        bind('wrap-llvm', self['swch_options_wrap_llvm'], 'active')
        bind('wrap-source', self['swch_options_wrap_source'], 'active')
        bind('window-maximized', self['swch_options_maximized'], 'active')

        # show-contents and show-source only apply when the application is
        # first started. In any session, even if the user chooses to show/hide
        # those panels, the configuration setting should not be affected
        bind('show-contents', self['grd_contents'],
             'visible', bidirectional=False)
        bind('show-contents', self['swch_options_contents'], 'active')
        bind('show-source', self['grd_source'], 'visible', bidirectional=False)
        bind('show-source', self['swch_options_source'], 'active')

        self.options.connect('notify::font', self.on_font_changed)
        self.options.connect('notify::wrap-llvm', self.on_wrap_llvm_changed)
        self.options.connect('notify::wrap-source',
                             self.on_wrap_source_changed)

    def _bind_widget_properties(self):
        self._bind(self['mitm_view_contents_pane'], 'active',
                   self['grd_contents'], 'visible')
        self._bind(self['mitm_view_source_pane'], 'active',
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

        # FIXME: At some point, implement goto line: # self['mitm_goto_line'],
        for widget in (self['mitm_reload'],
                       self['tlbtn_reload'],
                       self['mitm_close'],
                       self['tlbtn_close'],
                       self['mitm_search_forward'],
                       self['mitm_search_backward'],
                       self['mitm_search_contents']):
            bind('module', widget, 'sensitive')

        for widget in (self['mitm_push_mark'],
                       self['tlbtn_push_mark']):
            bind('entity', widget, 'sensitive')

        for widget in (self['tlbtn_source'],
                       self['mitm_view_source']):
            bind('entity-with-source', widget, 'sensitive')

        for widget in (self['tlbtn_goto_definition'],
                       self['mitm_goto_definition']):
            bind('entity-with-def', widget, 'sensitive')

        # for widget in (self['mitm_goto_prev_mark'],
        #                self['tlbtn_goto_prev_mark'],
        #                self['mitm_goto_next_mark'],
        #                self['tlbtn_goto_next_mark']):
        #     bind('mark', widget, 'sensitive')

        for widget in (self['mitm_pop_mark'],
                       self['tlbtn_pop_mark']):
            bind('mark', widget, 'sensitive')

        for widget in (self['mitm_goto_prev_use'],
                       self['tlbtn_goto_prev_use'],
                       self['mitm_goto_next_use'],
                       self['tlbtn_goto_next_use']):
            bind('mark-uses-count', widget, 'sensitive')

    def fn_contents_sort_names(self,
                               model: Gtk.TreeModel,
                               l: Gtk.TreeIter,
                               r: Gtk.TreeIter,
                               data: object):
        col_obj = ModelColsContents.Entity
        col_name = ModelColsContents.Display
        if model[l][col_obj] and model[r][col_obj]:
            left = model[l][col_name]
            right = model[r][col_name]
            if left < right:
                return -1
            elif left == right:
                return 0
            else:
                return 1
        return 0

    def fn_contents_filter_names(self,
                                 model: Gtk.TreeModel,
                                 i: Gtk.TreeIter,
                                 data: object) -> bool:
        if not model[i][ModelColsContents.Entity]:
            return True

        srch = self['srch_contents'].get_text()
        if not srch:
            return True

        name = model[i][ModelColsContents.Display]
        return srch in name

    # Utilities

    def format_tooltip(self, entity: int) -> str:
        source_name = lb.entity_get_source_name(entity)

        out = []
        if source_name:
            out.append('<span font_desc="{}">'.format(
                self.options.font.to_string()))
            out.append('<b>{:7}</b> {}'.format(
                'Source',
                GLib.markup_escape_text(source_name)))
            qualified_name = lb.entity_get_qualified_name(entity)
            if qualified_name:
                out.append('\n<b>{:7}</b> {}'.format(
                    'Qual',
                    GLib.markup_escape_text(qualified_name)))
            full_name = lb.entity_get_full_name(entity)
            if full_name:
                out.append('\n<b>{:7}</b> {}'.format(
                    'Full',
                    GLib.markup_escape_text(full_name)))
            out.append('</span>')

        return ''.join(out)


    @GObject.Signal
    def launch(self, *args):
        self['win_main'].show()
        if self.options.window_maximized:
            self['win_main'].maximize()

    # Implementation functions

    # The callbacks functions could be invoked by different means, either a
    # menu click, a keyboard shortcut or may be even implicitly as a result
    # of some other action. The do_* methods implement the actual funtionality
    # so the callback functions just process the argument as necessary

    def do_async(self, fn, *args):
        def impl(*args):
            fn(*args)
            return False

        GLib.idle_add(impl, *args, priority=GLib.PRIORITY_DEFAULT_IDLE)

    def do_clear(self):
        self.srcbuf_llvm.set_text('')
        self.srcbuf_code.set_text('')
        self['lbl_source_filename'].set_text('')
        self.trst_contents.clear()

    def do_scroll_llvm_to_offset(self, offset: int, length=0):
        def update_ui(i: Gtk.TreeIter) -> bool:
            self.srcvw_llvm.scroll_to_iter(i, 0.1, True, 0, 0)
            return False

        off_iter = self.srcbuf_llvm.get_iter_at_offset(offset)
        line = off_iter.get_line()
        column = off_iter.get_line_offset()
        line_iter = self.srcbuf_llvm.get_iter_at_line(line)
        col_iter = self.srcbuf_llvm.get_iter_at_line_offset(line, column)
        self.srcbuf_llvm.place_cursor(col_iter)
        if length:
            end_iter = self.srcbuf_llvm.get_iter_at_line_offset(
                line, column + length)
            self.srcbuf_llvm.select_range(col_iter, end_iter)
        self.do_async(update_ui, line_iter)

    def do_entity_select(self, entity: int):
        defn = lb.entity_get_llvm_defn(entity)
        offset = lb.def_get_begin(defn)
        tag = lb.entity_get_tag(entity)
        # Move the offset forward once because the offset will always be the
        # start of the tag which will be a sentinel character and we don't
        # want to match that
        self.do_scroll_llvm_to_offset(offset + 1, len(tag) - 1)

    def do_toggle_expand_row(self, path: Gtk.TreePath):
        if self.trvw_contents.row_expanded(path):
            self.trvw_contents.collapse_row(path)
        else:
            self.trvw_contents.expand_row(path, True)

    def do_populate_contents(self):
        def get_style(entity: int) -> Pango.Style:
            if lb.entity_is_artificial(entity):
                return Pango.Style.ITALIC
            return Pango.Style.NORMAL

        def add_category(label: str) -> Gtk.TreeIter:
            return self.trst_contents.append(
                None, [lb.get_null_handle(),
                       label,
                       '',
                       Pango.Style.NORMAL,
                       Pango.Weight.BOLD,
                       ''])

        def add_entity(i: Gtk.TreeIter, entity: int):
            self.trst_contents.append(
                i, [entity,
                    lb.entity_get_llvm_name(entity),
                    self.format_tooltip(entity),
                    get_style(entity),
                    Pango.Weight.NORMAL,
                    font])

        self.trvw_contents.set_model(None)
        self.trst_contents.clear()

        module = self.app.module
        font = self.options.font.get_family()
        for label, entities in [('Aliases', lb.module_get_aliases(module)),
                                ('Functions', lb.module_get_functions(module)),
                                ('Globals', lb.module_get_globals(module)),
                                ('Structs', lb.module_get_structs(module))]:
            i = add_category(label)
            for entity in entities:
                add_entity(i, entity)

        self.trvw_contents.set_model(self.trsrt_contents)

    def do_open(self):
        self.srcbuf_llvm.set_text(lb.module_get_code(self.app.module))

        self.do_populate_contents()

    def do_show_source(self, entity: int):
        def get_source_lang(file: str) -> GtkSource.Language:
            _, _, ext = file.rpartition('.')
            if ext in ['c', 'c99', 'c11']:
                return self.lang_mgr.get_language('c')
            elif ext in ['cc', 'cpp', 'cxx', 'C', 'CC', 'CPP', 'CXX',
                         'c++', 'h', 'hh', 'hpp', 'hxx',
                         'tcc', 'txx']:
                return self.lang_mgr.get_language('cpp')
            elif ext in ['f', 'F', 'fpp', 'f90', 'F90', 'F95', 'f95',
                         'fort', 'F03', 'f03', 'f08', 'f08']:
                return self.lang_mgr.get_language('fortran')
            elif ext in ['cu']:
                return self.lang_mgr.get_language('cuda')
            elif ext in ['rs']:
                return self.lang_mgr.get_language('rst')
            elif ext in ['objc']:
                return self.lang_mgr.get_language('objc')
            else:
                return None

        # This is an attempt to highlight something "useful" in the source
        # It is not always easy to do because a single "source expression"
        # could map to multiple LLVM instructions. Ideally, we should find some
        # way to draw a some kind of visual indicator at the appropriate column
        # Until then, just find a "token" and highlight it
        def get_source_expr_length(line: str) -> int:
            r = re.compile('^(\\s*[A-Za-z0-9_]+).*$')
            m = r.match(line)
            if m:
                return len(m.group(1))
            return 1

        def update_ui(i: Gtk.TreeIter) -> bool:
            self.srcvw_code.scroll_to_iter(i, 0.1, True, 0, 0)
            return False

        src_info = lb.entity_get_source_defn(entity)
        file = src_info.file
        line = src_info.begin.line - 1
        column = src_info.begin.column - 1
        self.srcbuf_code.set_language(get_source_lang(file))

        # If the file is already being shown, don't reload it
        if self['lbl_source_filename'].get_text() != file:
            self['lbl_source_filename'].set_text(file)
            with open(file, 'r') as f:
                self.srcbuf_code.set_text(f.read())

        line_iter = self.srcbuf_code.get_iter_at_line(line)
        col_iter = self.srcbuf_code.get_iter_at_line_offset(line, column)
        self.srcbuf_code.place_cursor(col_iter)

        text = self.srcbuf_code.get_text(
            col_iter, self.srcbuf_code.get_iter_at_line(line+1), False)
        end_iter = self.srcbuf_code.get_iter_at_line_offset(
            line, column + get_source_expr_length(text))
        self.srcbuf_code.select_range(col_iter, end_iter)
        self.do_async(update_ui, line_iter)

    def do_search_start(self, direction: SearchDirection):
        cursor = self.srcbuf_llvm.get_property('cursor-position')
        size = len(self.srcbuf_llvm.get_property('text'))
        if cursor < 0 or cursor >= size:
            cursor = 0
        self.search = SearchState(
            direction,
            self.srcbuf_llvm.get_iter_at_offset(cursor))
        self['srchbar_llvm'].set_search_mode(True)
        self['srch_llvm'].grab_focus()

    def do_search(self, srch: str) -> Tuple[Gtk.TextIter, Gtk.TextIter]:
        if self.search.direction == SearchDirection.Forward:
            match = self.search.iter_curr.forward_search(
                srch, self.search.flags, None)
            if match:
                self.search.iter_curr = match[1]
        else:
            match = self.search.iter_curr.backward_search(
                srch, self.search.flags, None)
            if match:
                self.search.iter_curr = match[0]
        if match:
            return match
        return None, None

    # Callback functions

    def on_open_recent(self, widget: Gtk.Widget) -> bool:
        file, _ = GLib.filename_from_uri(widget.get_current_item().get_uri())
        if file:
            self.app.action_open(file)
        return False

    def on_open(self, widget: Gtk.Widget) -> bool:
        dlg = self['dlg_file']
        response = dlg.run()
        dlg.hide()
        if response == Gtk.ResponseType.OK:
            # FIXME: Do this in a separate thread instead of blocking the
            # UI by doing this here
            self.app.action_open(dlg.get_filename())
        return False

    def on_close(self, *args) -> bool:
        self.app.action_close()
        self.do_clear()
        return False

    def on_reload(self, *args) -> bool:
        # FIXME: Do this in a separate thread instead of blocking the UI
        # by doing this
        self.app.action_reload()
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

    def on_contents_search_start(self, *args) -> bool:
        srchbar = self['srchbar_contents']
        if not srchbar.get_search_mode():
            self['srchbar_contents'].set_search_mode(True)
        else:
            self['srch_contents'].grab_focus()
        return False

    def on_contents_search_changed(self,
                                   srch: Gtk.SearchEntry) -> bool:
        self.trfltr_contents.refilter()
        return False

    def on_contents_search_focus_out(self, *args) -> bool:
        # If we tab out from the search entry, set the focus to the contents
        # view because that is almost certainly what we want
        self.trvw_contents.grab_focus()
        return False

    def on_contents_key_release(self,
                                trvw: Gtk.TreeView,
                                evt: Gdk.EventKey) -> bool:
        keys = set([Gdk.KEY_Return,
                    Gdk.KEY_KP_Enter,
                    Gdk.KEY_ISO_Enter])
        if evt.keyval in keys:
            model, i = self['trsel_contents'].get_selected()
            if i:
                path = model.get_path(i)
                entity = model[i][ModelColsContents.Entity]
                if entity:
                    self.do_entity_select(entity)
        elif evt.keyval == Gdk.KEY_Escape:
            if self['srchbar_contents'].get_search_mode():
                self['srchbar_contents'].set_search_mode(False)
        return False

    def on_contents_toggle_expand_all(self,
                                      tvcol: Gtk.TreeViewColumn) -> bool:
        if tvcol.get_title() == '+':
            self.trvw_contents.expand_all()
            tvcol.set_title('-')
        else:
            self.trvw_contents.collapse_all()
            tvcol.set_title('+')
        return False

    def on_contents_activated(self,
                              trvw: Gtk.TreeView,
                              path: Gtk.TreePath,
                              col: Gtk.TreeViewColumn) -> bool:
        model = trvw.get_model()
        entity = model[model.get_iter(path)][ModelColsContents.Entity]
        if entity:
            self.do_entity_select(entity)
        else:
            self.do_toggle_expand_row(path)

    def on_cursor_moved(self, obj: Gtk.TextBuffer, param: GObject.ParamSpec):
        offset = self.srcbuf_llvm.get_property('cursor-position')
        entity = lb.module_get_use_at(self.app.module, offset)
        if not entity:
            entity = lb.module_get_def_at(self.app.module, offset)
        if not entity:
            entity = lb.module_get_comdat_at(self.app.module, offset)
        self.app.entity = entity

        # If we can find an instruction under the cursor, we don't need to
        # look for a function because the function can be obtained from
        # the instruction, but if there is no instruction, we may still be
        # able to find a function
        self.app.inst = lb.module_get_instruction_at(self.app.module, offset)
        if not self.app.inst:
            self.app.func = lb.module_get_function_at(self.app.module, offset)

        return False

    def on_goto_line(self, *args) -> bool:
        return False

    def on_goto_definition(self, *args) -> bool:
        self.app.action_goto_definition()
        return False

    def on_goto_prev_use(self, *args) -> bool:
        self.app.action_goto_prev_use()
        return False

    def on_goto_next_use(self, *args) -> bool:
        self.app.action_goto_next_use()
        return False

    def on_goto_prev_mark(self, *args) -> bool:
        return False

    def on_goto_next_mark(self, *args) -> bool:
        return False

    def on_mark_push(self, *args) -> bool:
        offset = self.srcbuf_llvm.get_property('cursor-position')
        self.app.action_mark_push(self.app.entity, offset)

        return False

    def on_mark_pop(self, *args) -> bool:
        self.app.action_mark_pop()

        return False

    def on_show_source(self, *args) -> bool:
        self.app.action_show_source()
        return False

    def on_search_forward(self, *args) -> bool:
        self.do_search_start(SearchDirection.Forward)

        return False

    def on_search_backward(self, *args) -> bool:
        self.do_search_start(SearchDirection.Backward)

        return False

    def on_search_go(self, *args) -> bool:
        srch = self['srch_llvm'].get_text()
        if srch:
            m_begin, m_end = self.do_search(srch)
            if m_begin and m_end:
                self.do_scroll_llvm_to_offset(m_begin.get_offset(), len(srch))
        return False

    def on_search_stop(self, *args) -> bool:
        self.search = None
        self.srcvw_llvm.grab_focus()

        return False

    def on_srcvw_llvm_populate_popup(self, *args) -> bool:
        # Don't show the default popup. At some point, we may want to add
        # our own entries here
        print(*args)
        return True

    def on_toggle_fullscreen(self, *args) -> bool:
        if self['mitm_toggle_fullscreen'].get_active():
            self.win_main.fullscreen()
        else:
            self.win_main.unfullscreen()
        return False

    def on_font_changed(self, *args):
        font = self.options.font
        css = ['textview {']
        css.append('font-family: {};'.format(font.get_family()))
        css.append('font-size: {}pt;'.format(font.get_size() / Pango.SCALE))
        css.append('}')
        provider = Gtk.CssProvider.get_default()
        provider.load_from_data('\n'.join(css).encode('utf-8'))
        for widget in (self.srcvw_llvm,
                       self.srcvw_code):
            style = widget.get_style_context()
            style.add_provider(
                provider,
                Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)

    def on_wrap_llvm_changed(self, *args):
        wrap = Gtk.WrapMode.WORD \
            if self.options.wrap_llvm \
            else Gtk.WrapMode.NONE
        self.srcvw_llvm.set_wrap_mode(wrap)

    def on_wrap_source_changed(self, *args):
        wrap = Gtk.WrapMode.WORD \
            if self.options.wrap_source \
            else Gtk.WrapMode.NONE
        self.srcvw_code.set_wrap_mode(wrap)

    def get_application_window(self) -> Gtk.ApplicationWindow:
        return self.win_main

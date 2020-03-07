#!/usr/bin/env python3

from enum import IntEnum
import llvm_browse as lb
import operator
import os
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


# This is a tag applied to some range of the text. A tag will correspond to a
# single entity. There could be several tags that apply at any one source
# location. Tags may be completely nested within another, but they may not
# partially overlap

# Base class for the other tags that will be used here
class LLVMTag(GtkSource.Tag):
    start = GObject.Property(
        type=int,
        default=0,
        nick='start',
        blurb='The start offset for this tag')

    end = GObject.Property(
        type=int,
        default=0,
        nick='end',
        blurb='The end offset for this tag')

    use = GObject.Property(
        type=GObject.GObject,
        default=None,
        nick='use',
        blurb='The head of the use list for this definition')

    def __init__(self, llvm_range):
        GtkSource.Tag.__init__(self)

        self.start = llvm_range.begin
        self.end = llvm_range.end


class DefinitionTag(LLVMTag):
    entity = GObject.Property(
        type=GObject.TYPE_UINT64,
        default=None,
        nick='entity',
        blurb='The entity at this definition')

    def __init__(self, llvm_range, entity: int):
        LLVMTag.__init__(self, llvm_range)

        self.entity = entity


class UseTag(LLVMTag):
    defn = GObject.Property(
        type=DefinitionTag,
        default=None,
        nick='defn',
        blurb='The definition associated with this use')

    prev = GObject.Property(
        type=LLVMTag,
        default=None,
        nick='prev',
        blurb='The previous use in the chain')

    next = GObject.Property(
        type=LLVMTag,
        default=None,
        nick='next',
        blurb='The next use in the chain')

    def __init__(self,
                 llvm_range,
                 defn: DefinitionTag,
                 prev: 'UseTag' = None):
        LLVMTag.__init__(self, llvm_range)

        self.defn = defn
        self.prev = prev
        if self.prev:
            self.prev.next = self


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

        self.options.connect('notify::font', self.on_font_changed)

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
            bind('module', widget, 'sensitive')

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

    def do_entity_select(self, entity: int):
        def update_ui(i: Gtk.TreeIter) -> bool:
            self.srcvw_llvm.scroll_to_iter(i, 0.1, True, 0, 0)
            return False

        offset = lb.entity_get_llvm_defn(entity).begin
        off_iter = self.srcbuf_llvm.get_iter_at_offset(offset)
        line = off_iter.get_line()
        line_iter = self.srcbuf_llvm.get_iter_at_line(line)
        self.srcbuf_llvm.place_cursor(line_iter)
        self.do_async(update_ui, line_iter)

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

        def get_tooltip(entity: int) -> str:
            source_name = lb.entity_get_source_name(entity)
            full_name = lb.entity_get_full_name(entity)

            out = []
            if source_name:
                out.append('<span font_desc="{}">'.format(
                    self.options.font.to_string()))
                if source_name:
                    out.append('<b>{:7}</b> {}'.format(
                        'Source',
                        GLib.markup_escape_text(source_name)))
                if full_name:
                    # We will never have the situation where an entity has a
                    # full name but no source name because the full name will
                    # have been computed from the source name. So it's safe to
                    # prepend the string with a newline
                    out.append('\n<b>{:7}</b> {}'.format(
                        'Full',
                        GLib.markup_escape_text(full_name)))
                out.append('</span>')

            return ''.join(out)

        def add_category(label: str) -> Gtk.TreeIter:
            return self.trst_contents.append(
                None, [lb.HANDLE_NULL,
                       label,
                       '',
                       Pango.Style.NORMAL,
                       Pango.Weight.BOLD,
                       ''])

        def add_entity(i: Gtk.TreeIter, entity: int):
            self.trst_contents.append(
                i, [entity,
                    lb.entity_get_llvm_name(entity),
                    get_tooltip(entity),
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

    def do_compute_tags(self):
        def add_tag(tag: LLVMTag) -> LLVMTag:
            self.tag_table.add(tag)
            self.srcbuf_llvm.apply_tag(
                tag,
                self.srcbuf_llvm.get_iter_at_offset(tag.start),
                self.srcbuf_llvm.get_iter_at_offset(tag.end))

            return tag

        def add_tags(entity: int):
            defn = lb.entity_get_llvm_defn(entity)
            # If we can't get a definition for the entity, don't bother trying 
            # to mark any uses
            if defn:
                defn_tag = add_tag(DefinitionTag(defn, entity))
                prev_use = None
                for use in lb.entity_get_uses(entity):
                    use_tag = add_tag(UseTag(use, defn_tag, prev_use))
                    prev_use = use_tag
                    if not defn_tag.use:
                        defn_tag.use = use_tag

        module = self.app.module
        for f in lb.module_get_functions(module):
            add_tags(f)
            for arg in lb.func_get_args(f):
                add_tags(arg)
            for bb in lb.func_get_blocks(f):
                add_tags(bb)
                for inst in lb.block_get_instructions(bb):
                    add_tags(inst)

        for entities in (lb.module_get_aliases(module),
                         lb.module_get_globals(module)):
            for entity in entities:
                add_tags(entity)

    def do_open(self):
        self.srcbuf_llvm.set_text(lb.module_get_code(self.app.module))

        self.do_populate_contents()
        self.do_compute_tags()

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

    def on_srcvw_llvm_button(self,
                             srcvw: GtkSource.View,
                             evt: Gdk.EventButton) -> bool:
        if (evt.type == Gdk.EventType.BUTTON_RELEASE) and (evt.button == 1):
            x, y = srcvw.window_to_buffer_coords(
                Gtk.TextWindowType.TEXT, evt.x, evt.y)
            over_text, i = srcvw.get_iter_at_location(x, y)
            if over_text:
                for tag in i.get_tags():
                    if isinstance(tag, DefinitionTag):
                        print('Got definition', tag.entity.source_name)
                    elif isinstance(tag, UseTag):
                        print('Got use', tag)
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

    def get_application_window(self) -> Gtk.ApplicationWindow:
        return self.win_main

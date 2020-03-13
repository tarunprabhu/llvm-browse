#!/usr/bin/env python3

from .logger import critical, error, message, warning
from os import makedirs, path
from sys import stderr
from typing import List
import gi
gi.require_version('GLib', '${PY_GLIB_VERSION}')
gi.require_version('GObject', '${PY_GOBJECT_VERSION}')
gi.require_version('Gtk', '${PY_GTK_VERSION}')
gi.require_version('GtkSource', '${PY_GTKSOURCE_VERSION}')
gi.require_version('Pango', '${PY_PANGO_VERSION}')
from gi.repository import GLib, GObject, Gtk, GtkSource, Pango  # NOQA: E402


class Options(GObject.GObject):
    font = GObject.Property(
        type=Pango.FontDescription,
        default=Pango.FontDescription.from_string('Monospace 10'),
        nick='font',
        blurb='Font to use in the source views')

    style = GObject.Property(
        type=GtkSource.StyleScheme,
        default=GtkSource.StyleSchemeManager.get_default().get_scheme('classic'),
        nick='style',
        blurb='The style scheme to use in the source views')

    style_llvm = GObject.Property(
        type=GtkSource.StyleScheme,
        default=style,
        nick='style-llvm',
        blurb=('If syntax_llvm is True, this will be the same as "style" '
               'else None'))

    style_source = GObject.Property(
        type=GtkSource.StyleScheme,
        default=style,
        nick='style-source',
        blurb=('If syntax_source is True, this will be the same as "style" '
               'else None'))

    syntax_llvm = GObject.Property(
        type=bool,
        default=True,
        nick='syntax-llvm',
        blurb='Use syntax highlighting of the LLVM code')

    syntax_source = GObject.Property(
        type=bool,
        default=False,
        nick='syntax-source',
        blurb='Use syntax highlighting of the source code')

    line_nums_llvm = GObject.Property(
        type=bool,
        default=False,
        nick='line-nums-llvm',
        blurb='Show line numbers in the LLVM view')

    line_nums_source = GObject.Property(
        type=bool,
        default=True,
        nick='line-nums-source',
        blurb='Show line numbers in the source view')

    wrap_llvm = GObject.Property(
        type=bool,
        default=False,
        nick='wrap-llvm',
        blurb='True if lines should be wrapped in the LLVM view')

    wrap_source = GObject.Property(
        type=bool,
        default=False,
        nick='wrap-source',
        blurb='True if lines should be wrapped in the source view')

    show_contents = GObject.Property(
        type=bool,
        default=True,
        nick='show-contents',
        blurb='Show the contents view on startup')

    show_source = GObject.Property(
        type=bool,
        default=True,
        nick='show-source',
        blurb='Show the source view on startup')

    window_maximized = GObject.Property(
        type=bool,
        default=False,
        nick='window-maximized',
        blurb='The window state on startup')

    max_marks = GObject.Property(
        type=int,
        default=1024,
        nick='max-marks',
        blurb='The maximum number of marks that can be added')

    @classmethod
    def get_properties(cls) -> List[GObject.Property]:
        return [p for p in cls.__dict__.values()
                if isinstance(p, GObject.Property)]

    def __init__(self, app):
        GObject.GObject.__init__(self)

        self.app = app
        self.file_path = path.join(GLib.get_user_config_dir(),
                                   GLib.get_prgname(),
                                   GLib.get_prgname() + '.conf')

        self.read_funcs = {
            'PangoFontDescription': self._get_font_description,
            'GtkSourceStyleScheme': self._get_style_scheme,
            'gboolean': self._get_boolean,
            'gint': self._get_int,
            'gint64': self._get_int,
            'guint': self._get_unsigned,
            'guint64': self._get_unsigned,
            'gchararray': self._get_string
        }

        self.write_funcs = {
            'PangoFontDescription': self._set_font_description,
            'GtkSourceStyleScheme': self._set_style_scheme,
            'gboolean': self._set_boolean,
            'gint': self._set_int,
            'gint64': self._set_int,
            'guint': self._set_unsigned,
            'guint64': self._set_unsigned,
            'gchararray': self._set_string
        }

        self.kf = GLib.KeyFile.new()

        self.connect('notify::syntax-llvm', self.on_syntax_llvm_changed)
        self.connect('notify::syntax-source', self.on_syntax_source_changed)
        self.bind_llvm: GObject.Binding = self.bind_property(
            'style', self, 'style-llvm',
            GObject.BindingFlags.DEFAULT | GObject.BindingFlags.SYNC_CREATE)
        self.bind_source: GObject.Binding = self.bind_property(
            'style', self, 'style-source',
            GObject.BindingFlags.DEFAULT | GObject.BindingFlags.SYNC_CREATE)

    def on_syntax_llvm_changed(self, *args):
        if self.bind_llvm:
            self.bind_llvm.unbind()
            self.bind_llvm = None
            self.style_llvm = None
        if self.syntax_llvm:
            self.bind_llvm = self.bind_property(
                'style', self, 'style-llvm',
                GObject.BindingFlags.DEFAULT | GObject.BindingFlags.SYNC_CREATE)

    def on_syntax_source_changed(self, *args):
        if self.bind_source:
            self.bind_source.unbind()
            self.bind_source = None
            self.style_source = None
        if self.syntax_source:
            self.bind_source = self.bind_property(
                'style', self, 'style-source',
                GObject.BindingFlags.DEFAULT | GObject.BindingFlags.SYNC_CREATE)

    def _get_start_group(self) -> str:
        grp = self.kf.get_start_group()
        if not grp:
            grp = 'llvm-browse'
        return grp

    def _get_impl(self, prop: GObject.Property, fn):
        return fn(self._get_start_group(), prop.nick)

    def _get_font_description(self,
                              prop: GObject.Property) -> Pango.FontDescription:
        return Pango.FontDescription.from_string(
            self._get_impl(prop, self.kf.get_string))

    def _get_style_scheme(self,
                          prop: GObject.Property) -> GtkSource.StyleScheme:
        mgr = GtkSource.StyleSchemeManager.get_default()
        id = self._get_impl(prop, self.kf.get_string)
        scheme = mgr.get_scheme(id)
        if scheme is None:
            print('Could not find style scheme:', id, '. Reverting to default',
                  file=stderr)
            scheme = Options.style.default
        return scheme

    def _get_int(self, prop: GObject.Property) -> int:
        return self._get_impl(prop, self.kf.get_int64)

    def _get_unsigned(self, prop: GObject.Property) -> int:
        return self._get_impl(prop, self.kf.get_uint64)

    def _get_boolean(self, prop: GObject.Property) -> bool:
        return self._get_impl(prop, self.kf.get_boolean)

    def _get_string(self, prop: GObject.Property) -> str:
        return self._get_impl(prop, self.kf.get_string)

    def _set_impl(self, prop: GObject.Property, fn_set, val):
        grp = self._get_start_group()
        fn_set(grp, prop.nick, val)
        self.kf.set_comment(grp, prop.nick, prop.blurb)

    def _set_font_description(self, prop: GObject.Property):
        self._set_impl(prop, self.kf.set_string,
                       self.get_property(prop.name).to_string())

    def _set_style_scheme(self, prop: GObject.Property):
        if self.get_property(prop.name):
            self._set_impl(prop, self.kf.set_string,
                           self.get_property(prop.name).get_id())
        else:
            self._set_impl(prop, self.kf.set_string, '')

    def _set_boolean(self, prop: GObject.Property):
        self._set_impl(prop, self.kf.set_boolean, self.get_property(prop.name))

    def _set_int(self, prop: GObject.Property):
        self._set_impl(prop, self.kf.set_int64, self.get_property(prop.name))

    def _set_unsigned(self, prop: GObject.Property):
        self._set_impl(prop, self.kf.set_uint64, self.get_property(prop.name))

    def _set_string(self, prop: GObject.Property):
        self._set_impl(prop, self.kf.set_string, self.get_property(prop.name))

    def load(self):
        try:
            message('Reading options from', self.file_path)
            self.kf.load_from_file(
                self.file_path, GLib.KeyFileFlags.KEEP_COMMENTS)
            for prop in Options.get_properties():
                try:
                    self.set_property(
                        prop.name,
                        self.read_funcs[prop.type.name](prop))
                except GLib.Error as e:
                    warning(e.message)
                    self.set_property(prop.name,
                                      getattr(Options, prop.name).default)
        except GLib.Error as ferr:
            critical('Could not open config file.', ferr.message, file=stderr)

    def store(self):
        for prop in Options.get_properties():
            self.write_funcs[prop.type.name](prop)
        try:
            message('Writing options to', self.file_path)
            if not path.exists(self.file_path):
                makedirs(path.dirname(self.file_path))
            self.kf.save_to_file(self.file_path)
        except GLib.Error as ferr:
            critical('Could not write config file.', ferr.message, file=stderr)

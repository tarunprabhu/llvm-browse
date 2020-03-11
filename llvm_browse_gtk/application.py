#!/usr/bin/env python3

import argparse
from typing import List, Mapping
import llvm_browse as lb
from .options import Options
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
    module = GObject.Property(
        type=GObject.TYPE_UINT64,
        default=lb.get_null_handle(),
        nick='module',
        blurb='Handle to the LLVM module')

    llvm = GObject.Property(
        type=str,
        default='',
        nick='llvm',
        blurb='The path to the LLVM file currently shown or ""')

    entity = GObject.Property(
        type=GObject.TYPE_UINT64,
        default=lb.get_null_handle(),
        nick='entity',
        blurb=('The current entity. '
               'This will be a use, def or comdat'))

    inst = GObject.Property(
        type=GObject.TYPE_UINT64,
        default=lb.get_null_handle(),
        nick='inst',
        blurb=('The current instruction'))

    func = GObject.Property(
        type=GObject.TYPE_UINT64,
        default=lb.get_null_handle(),
        nick='func',
        blurb=('The current function. '
               'This may be set even if self.inst is not'))

    # entity_with_source will be the same as the instruction
    # if self.inst is set else it will be self.func
    entity_with_source = GObject.Property(
        type=GObject.TYPE_UINT64,
        default=lb.get_null_handle(),
        nick='entity-with-source',
        blurb=('Handle of the entity whose source will be shown '
               'when the show-source action is launched.'))

    # entity_with_def will be the same as self.entity
    # if self.entity is a use or a comdat. Else it will be None
    entity_with_def = GObject.Property(
        type=GObject.TYPE_UINT64,
        default=lb.get_null_handle(),
        nick='entity-with-def',
        blurb=('Handle of the entity with a definition. '
               'Used when the goto definition action is launched'))

    mark = GObject.Property(
        type=GObject.TYPE_UINT64,
        default=lb.get_null_handle(),
        nick='curr-mark',
        blurb=('Handle of the marked entity. '
               'This is the handle at the top of the self.marks stack'))

    mark_uses_count = GObject.Property(
        type=int,
        default=0,
        nick='mark-uses-count',
        blurb=('The number of uses for the currently marked entity'))

    mark_uses_index = GObject.Property(
        type=int,
        default=-1,
        nick='mark-uses-index',
        blurb=('The index into the use list for the currently marked entity. '
               'This is the count at the top of the self.marks stack'))

    def __init__(self):
        Gtk.Application.__init__(self)

        GLib.set_application_name('LLVM Browse')
        GLib.set_prgname('llvm-browse')

        self.argv: argparse.Namespace = None
        self.options: Options = Options(self)
        self.ui: UI = UI(self)

        # The user has to explicitly set a mark. When one is set, prev-use
        # and next-use will be enabled and the user can navigate this list
        self.marks: List[int] = []

        # A map from entities to uses. The keys are all guaranteed to be in
        # self.marks
        self.uses_map: Mapping[int, List[int]] = {}

        # Map from the entities with marks set to the index of the use that
        # was last jumped to using prev-us or next-use. Because the same
        # entity can be marked more than once, the value is a list
        self.uses_indexes_map: Mapping[int, List[int]] = {}

        self.connect('notify::entity', self.on_entity_changed)
        self.connect('notify::inst', self.on_instruction_changed)
        self.connect('notify::func', self.on_function_changed)

    def _reset(self):
        if self.module:
            lb.module_free(self.module)
        self.module = lb.get_null_handle()
        self.llvm = ''

    def do_activate(self, *args) -> bool:
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
        self.module = lb.module_create(file)
        if not self.module:
            self._reset()
        else:
            self.ui.do_open()
        return bool(self.module)

    # Returns true if the file could be closed
    def action_close(self) -> bool:
        self._reset()
        return True

    # Returns true if the file could be reloaded
    def action_reload(self) -> bool:
        if self.llvm:
            llvm = self.llvm
            self._reset()
            return self.action_open(llvm)
        return False

    # Returns true on success. Not sure if this will actually return
    def action_quit(self) -> bool:
        self._reset()
        self.remove_window(self.ui.get_application_window())
        return True

    def action_goto_definition(self) -> bool:
        if self.entity_with_def:
            defn = lb.entity_get_llvm_defn(self.entity_with_def)
            offset = lb.def_get_begin(defn)
            self.ui.do_scroll_llvm_to_offset(offset)
            return True
        return False

    def action_goto_prev_use(self) -> bool:
        if self.marks:
            if self.mark_uses_count:
                # If there is only a single use, the "previous use" is the same
                # as the only use even if we are already at that single use
                if self.mark_uses_count > 1:
                    if self.mark_uses_index == 0:
                        self.mark_uses_index = self.mark_uses_count
                    self.mark_uses_index -= 1
                use = self.uses_map[self.mark][self.mark_uses_index]
                self.ui.do_scroll_llvm_to_offset(lb.use_get_begin(use))
                return True
            return False
        return False

    def action_goto_next_use(self) -> bool:
        if self.marks:
            if self.mark_uses_count:
                # If there is only a single use, the "next use" is the same
                # as the only use even if we are already at that single use
                if self.mark_uses_count > 1:
                    if self.mark_uses_index == self.mark_uses_count - 1:
                        self.mark_uses_index = -1
                    self.mark_uses_index += 1
                use = self.uses_map[self.mark][self.mark_uses_index]
                self.ui.do_scroll_llvm_to_offset(lb.use_get_begin(use))
            return False
        return False

    def action_go_back(self) -> bool:
        pass

    def action_go_forward(self) -> bool:
        pass

    def action_show_source(self) -> bool:
        if self.entity_with_source:
            self.ui.do_show_source(self.entity_with_source)
            return True
        return False

    def set_mark(self):
        entity = self.marks[-1] if self.marks else lb.get_null_handle()
        if lb.is_null_handle(entity):
            self.mark = lb.get_null_handle()
            self.mark_uses_count = 0
            self.mark_uses_index = -1
        else:
            self.mark = entity
            self.mark_uses_count = len(self.uses_map[entity])
            self.mark_uses_index = self.uses_indexes_map[entity][-1]

    def action_mark_push(self, entity: int) -> bool:
        if len(self.marks) == self.options.max_marks:
            return False

        if lb.is_null_handle(entity) or not lb.is_use(entity):
            # If the entity being pushed is not a use, add a mark anyway
            # This mark will be used for navigation only
            self.marks.append(lb.get_null_handle())
        elif lb.is_use(entity):
            self.marks.append(entity)
            if entity not in self.uses_indexes_map:
                self.uses_map[entity] = lb.entity_get_uses(entity)
                self.uses_indexes_map[entity] = []
            self.uses_indexes_map[entity].append(
                0 if self.uses_map[entity] else -1)
        self.set_mark()
        return True

    def action_mark_pop(self) -> bool:
        if not self.marks:
            return False

        # If there is at least one mark, pop it
        if self.marks:
            entity = self.marks.pop()
            # If the mark is actually a use, then remove all the metadata
            # associated with it if it is the last instance of the mark
            if not lb.is_null_handle(entity):
                self.uses_indexes_map[entity].pop()
                if len(self.uses_indexes_map) == 0:
                    del self.uses_indexes_map[entity]
                    del self.uses_map[entity]
        self.set_mark()
        return True

    def on_entity_changed(self, *args):
        if self.entity:
            if lb.is_use(self.entity):
                self.entity_with_def = lb.use_get_used(self.entity)
            elif lb.is_def(self.entity):
                self.entity_with_def = lb.get_null_handle()
            elif lb.is_comdat(self.entity):
                self.entity_with_def = self.entity

    def on_instruction_changed(self, *args):
        if self.inst:
            if lb.inst_has_source_info(self.inst):
                self.entity_with_source = self.inst
            self.func = lb.inst_get_function(self.inst)

    def on_function_changed(self, *args):
        if self.func:
            if not self.inst:
                if lb.func_has_source_info(self.func):
                    self.entity_with_source = self.func

    def run(self, argv: argparse.Namespace) -> int:
        self.argv = argv
        ret = Gtk.Application.run(self)
        self.options.store()
        return ret

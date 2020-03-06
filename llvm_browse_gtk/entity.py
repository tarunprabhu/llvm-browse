#!/usr/bin/env python3

import llvm_browse as lb

import gi
gi.require_version('GObject', '${PY_GOBJECT_VERSION}')
from gi.repository import GObject  # NOQA: E402


class Entity(GObject.GObject):
    handle = GObject.Property(
        type=GObject.TYPE_UINT64,
        default=0,
        nick='handle',
        blurb='Handle generated by llvm_browse')

    tag = GObject.Property(
        type=str,
        default='',
        nick='tag',
        blurb='The tag in the LLVM IR. This might be a slot')

    llvm_name = GObject.Property(
        type=str,
        default='',
        nick='llvm-name',
        blurb='The LLVM name for the entity. Guaranteed not to be a slot')

    used = GObject.Property(
        type=bool,
        default=False,
        nick='used',
        blurb='True if the entity has at least one use')

    has_source_info = GObject.Property(
        type=bool,
        default=False,
        nick='has-source-info',
        blurb='Whether or not this entity has debug information')

    source_name = GObject.Property(
        type=str,
        default='',
        nick='source-name',
        blurb='Name of the entity in the source')

    artificial = GObject.Property(
        type=bool,
        default=False,
        nick='artificial',
        blurb=('True if this property has no corresponding entity '
               'in the source code'))

    def __init__(self, handle: int):
        GObject.GObject.__init__(self)

        self.handle = handle
        self.tag = lb.entity_get_tag(self.handle)
        self.llvm_name = lb.entity_get_llvm_name(self.handle)
        self.has_source_info = lb.entity_has_source_info(self.handle)
        self.source_name = lb.entity_get_source_name(self.handle)
        self.llvm_defn = lb.entity_get_llvm_defn(self.handle)
        self.llvm_span = lb.entity_get_llvm_span(self.handle)
        self.source_defn = None
        self.source_span = None
        self.uses = lb.entity_get_uses(self.handle)
        self.used = bool(self.uses)
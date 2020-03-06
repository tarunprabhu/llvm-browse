#!/usr/bin/env python3

from .entity import Entity
import llvm_browse as lb

import gi
gi.require_version('GObject', '${PY_GOBJECT_VERSION}')
from gi.repository import GObject # NOQA: E402


class Global(Entity):
	full_name = GObject.Property(
		type=str,
		default='',
		nick='full-name',
		blurb='The full name including scope and type information')

	mangled = GObject.Property(
		type=bool,
		default=False,
		nick='mangled',
		blurb='True if the global name is mangled')

	def __init__(self, handle: int):
		Entity.__init__(self, handle)

		self.source_defn = lb.entity_get_source_defn(self.handle)
		self.source_span = lb.entity_get_source_span(self.handle)
		self.full_name = lb.global_get_full_name(self.handle)
		self.mangled = lb.global_is_mangled(self.handle)
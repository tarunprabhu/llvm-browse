#!/usr/bin/env python3

from .entity import Entity
import llvm_browse as lb

import gi
gi.require_version('GObject', '${PY_GOBJECT_VERSION}')
from gi.repository import GObject

# Represents an LLVM function
class Function(Entity):
	full_name = GObject.Property(
		type=str,
		default='',
		nick='full-name',
		blurb='Full name of the function in case the LLVM name is mangled')

	mangled = GObject.Property(
		type=bool,
		default=False,
		nick='mangled',
		blurb='True if the function has a mangled name')

	def __init__(self, handle: int):
		Entity.__init__(self, handle)

		self.source_defn = lb.entity_get_source_defn(self.handle)
		self.source_span = lb.entity_get_source_span(self.handle)
		self.full_name = lb.function_get_full_name(self.handle)
		self.artificial = lb.function_is_artificial(self.handle)
		self.mangled = lb.function_is_mangled(self.handle)

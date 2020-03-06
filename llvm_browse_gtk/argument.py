#!/usr/bin/env python3

from .entity import Entity
import llvm_browse as lb

import gi
gi.require_version('GObject', '${PY_GOBJECT_VERSION}')
from gi.repository import GObject # NOQA: E402


class Argument(Entity):
	artificial = GObject.Property(
		type=bool,
		default=False,
		nick='artificial',
		blurb='Argument that was implicitly inserted by the compiler')

	def __init__(self, handle: int):
		Entity.__init__(self, handle)

		self.artificial = lb.argument_is_artificial()
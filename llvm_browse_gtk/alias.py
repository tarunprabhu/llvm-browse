#!/usr/bin/env python3

from .entity import Entity

import gi
gi.require_version('GObject', '${PY_GOBJECT_VERSION}')
from gi.repository import GObject # NOQA: E402


class Alias(Entity):
	def __init__(self, handle: int):
		Entity.__init__(self, handle)

		self.artificial = True
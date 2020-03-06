#!/usr/bin/env python3

from .entity import Entity

import gi
gi.require_version('GObject', '${PY_GOBJECT_VERSION}')
from gi.repository import GObject # NOQA: E402


class Struct(Entity):
	full_name = GObject.Property(
		type=str,
		default='',
		nick='full-name',
		blurb='Full name of the struct including scope and types')

	def __init__(self, handle: int):
		Entity.__init__(self, handle)

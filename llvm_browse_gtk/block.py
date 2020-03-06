#!/usr/bin/env python3

from .entity import Entity
import llvm_browse as lb

import gi
gi.require_version('GObject', '${PY_GOBJECT_VERSION}')
from gi.repository import GObject # NOQA: E402


class Block(Entity):
	def __init__(self, handle: int):
		Entity.__init__(handle)

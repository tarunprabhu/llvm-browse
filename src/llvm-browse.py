#!/usr/bin/env python3

from typing import List
from application import LLVMBrowse
import argparse
import gi
gi.require_version('GObject', '2.0')
gi.require_version('Gtk', '3.0')
gi.require_version('GtkSource', '4')
from gi.repository import GObject, Gtk, GtkSource  # NOQA: E402


def parse_commandline_args() -> argparse.Namespace:
    ap = argparse.ArgumentParser(description='Browse LLVM IR')
    ap.add_argument('-x', '--maximize', action='store_true', default=False,
                    help='Maximize the window on startup')
    ap.add_argument('file', type=str, nargs='?', default='',
                    help='The LLVM IR file to open')
    return ap.parse_args()


def main(argv: List[str]) -> int:
    args: argparse.Namespace = parse_commandline_args()

    # Not sure why I need to register the GtkSourceView, but best to do it
    # here so we can be sure that it gets registered before anything else
    GObject.type_register(GtkSource.View)

    app = LLVMBrowse()
    app.run(argv)

    return 0


if __name__ == '__main__':
    from sys import argv
    exit(main(argv))

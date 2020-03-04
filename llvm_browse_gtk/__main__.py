#!/usr/bin/env python3

from .application import Application

import argparse

def main():
    ap = argparse.ArgumentParser(description='Browse LLVM IR')
    ap.add_argument('-x', '--maximize', action='store_true', default=False,
                    help='Maximize the window on startup')
    ap.add_argument('file', type=str, nargs='?', default='',
                    help='The LLVM IR file to open')
    argv = ap.parse_args()

    app = Application()
    app.run(argv)

    return 0


if __name__ == '__main__':
    main()

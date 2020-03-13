#!/usr/bin/env python3

import sys
# Colored output is nice to have, but not required.
try:
    from termcolor import cprint as _cprint
except:
    _cprint = None


def cprint(tag: str,
           tag_color: str, 
           msg: str, 
           *args, **kwargs):
    if _cprint:
        _cprint(tag, tag_color, attrs=['bold'], end='')
    else:
        print(tag.upper(), end='')
    print(':', end=' ')
    print(msg, *args, **kwargs)


def message(msg: str, *args, **kwargs):
    cprint('Message', 'green', msg, *args, **kwargs)


def warning(msg: str, *args, **kwargs):
    cprint('Warning', 'yellow', msg, *args, **kwargs)


def critical(msg: str, *args, **kwargs):
    cprint('Critical', 'magenta', msg, *args, **kwargs)


def error(msg: str, *args, **kwargs):
    cprint('Error', 'red', msg, *args, **kwargs)

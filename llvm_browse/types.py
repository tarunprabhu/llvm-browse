#!/usr/bin/env python3

from collections import namedtuple

SourceRange = namedtuple('SourceRange', ['line', 'column'])

# SourceLoc represents a location in the source file
#   file: full path to the source file
#   line: Line number
#   column: Column number
#
SourceLoc = namedtuple('SourceLoc',
                       [
                           'file',
                           'begin',
                           'end',
                       ])

# LLVMLoc represents a location in the LLVM file
# It is actually a range and represents the range in the LLVM file that an
# entity spans
#
LLVMLoc = namedtuple('LLVMLoc', ['begin', 'end'])

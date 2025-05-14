#!/usr/bin/env python3

# Reindents all the preprocessor directives.
#
# For example:
#
#   #ifdef FOO
#   #define BAR
#   void foo();
#   #else
#   #if BAZ
#   #define QUUX
#   #endif
#   #endif
#
# Would be transformed into:
#
#   #ifdef FOO
#   #  define BAR
#   void foo();
#   #else
#   #  if BAZ
#   #    define QUUX
#   #  endif
#   #endif
#
# This makes the indentation reflect the lexical structure of the preprocessor
# directives. Note that source code within is not re-indented!

from io import StringIO
import sys

INDENTATION = ' '*2

DIRECTIVES = (
    'define',
    'elif',
    'else',
    'endif',
    'if',
    'ifdef',
    'ifndef',
    'include',
    'pragma',
    'undef',
)

INDENT_AFTER   = {'if', 'ifdef', 'ifndef', 'else', 'elif'}
OUTDENT_BEFORE = {'endif', 'else', 'elif'}

def Process(infile, outfile):
    depth = 0
    for line in infile:
        if line.startswith('#'):
            rest = line[1:].lstrip()
            directive = rest.split()[0]
            assert directive in DIRECTIVES
            if directive in OUTDENT_BEFORE:
                assert depth > 0
                depth -= 1
            outfile.write('#' + (INDENTATION * depth) + rest)
            if directive in INDENT_AFTER:
                depth += 1
        else:
            outfile.write(line)
    assert depth == 0


if __name__ == '__main__':
    if len(sys.argv) < 2:
        Process(sys.stdin, sys.stdout)
    else:
        # Warning: modifies files in-place!
        for filename in sys.argv[1:]:
            result = StringIO()
            with open(filename, 'rt') as f:
                Process(f, result)

            with open(filename, 'wt') as f:
                f.write(result.getvalue())

#!/usr/bin/env python3
"""Find top-level C/C++ blocks that run into each other without a blank line.

Checked in Authum and LH sources (vendored code is skipped):

- the include guard ``#define`` and what follows it;
- the ``#include`` block and the code after it;
- a function's closing ``}`` and the next definition or test;
- a type's closing ``};`` and the next declaration (its own ``typedef`` and
  ``@copybrief`` line stay attached, as does a table's ``_size`` constant);
- a declaration in a header and the next one;
- the file comment and the include guard.

    python tools/check-layout.py          # report, exit 1 when anything is found
    python tools/check-layout.py --fix    # insert the missing blank lines

Line endings of every file are kept as they are.
"""

import os
import re
import sys

ROOTS = ('include', 'src', 'tests', 'examples', 'lib/lh/include', 'lib/lh/src',
         'lib/lh/test/src', 'lib/lh/bench', 'lib/lh/example')
SKIP = ('lib/sqlite', 'lib/monocypher', 'lib/hlsdk', '/build', 'third_party', 'googletest',
        '/lib/lh/lib/', 'tools/data')
GUARD = re.compile(r'#define [A-Z0-9_]+_H\b\s*$')
TOP_DECL = re.compile(r'^(?:static\s+|const\s+|struct\s+|enum\s+|unsigned\s+|signed\s+)*'
                      r'[A-Za-z_][\w\s\*]*$')


def find(lines, header):
    """Yield (index, reason) for each line that needs a blank line after it."""
    for i in range(len(lines) - 1):
        a, b = lines[i], lines[i + 1]
        if not a.strip() or not b.strip():
            continue
        if GUARD.match(a) and i > 0 and lines[i - 1].startswith('#ifndef'):
            yield i, 'guard'
        elif a.startswith('#include') and not b.startswith(('#', '//')):
            yield i, 'include'
        elif a == '}' and b != '}' and (b.startswith(('TEST', '/*')) or TOP_DECL.match(b)):
            yield i, 'after function'
        elif (re.match(r'^\}\s*\w*\s*;', a)
              and not b.startswith(('}', '#endif', 'typedef', '/** @copybrief', '/**<'))
              and '_size =' not in b):
            yield i, 'after type'
        elif (header and a.endswith(');') and not a.startswith((' ', '\t', '#'))
              and (b.startswith('/**') or TOP_DECL.match(b))):
            yield i, 'after declaration'
        elif (a == ' */' and b.startswith('#ifndef') and i + 2 < len(lines)
              and GUARD.match(lines[i + 2])):
            yield i, 'file comment'


def main():
    fix = '--fix' in sys.argv[1:]
    total = 0
    for root in ROOTS:
        for directory, _, names in os.walk(root):
            if any(s in directory.replace(os.sep, '/') + '/' for s in SKIP):
                continue
            for name in names:
                if not name.endswith(('.c', '.h', '.cpp', '.hpp')):
                    continue
                path = os.path.join(directory, name).replace(os.sep, '/')
                try:
                    text = open(path, 'rb').read().decode('utf-8')
                except UnicodeDecodeError:
                    continue
                newline = '\r\n' if '\r\n' in text else '\n'
                lines = text.replace('\r\n', '\n').split('\n')
                found = list(find(lines, path.endswith(('.h', '.hpp'))))
                for index, reason in found:
                    print(f'{path}:{index + 1}: no blank line {reason}')
                total += len(found)
                if fix and found:
                    for index, _ in reversed(found):
                        lines.insert(index + 1, '')
                    open(path, 'wb').write('\n'.join(lines).replace('\n', newline).encode())
    print(f'{total} place(s){" fixed" if fix else ""}.')
    return 0 if fix or not total else 1


if __name__ == '__main__':
    sys.exit(main())

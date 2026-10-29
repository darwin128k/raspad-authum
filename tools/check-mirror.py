#!/usr/bin/env python3
"""Check that sources mirror headers: the same folders, the same file names.

    python tools/check-mirror.py

A source `lib/lh/src/lh/<path>.c` implements `lib/lh/include/lh/<path>.h`, and a
module API source `src/authum/api/<path>.c` implements `include/authum/<path>.h`.
Authum unit tests in `tests/src/authum/<path>.cpp` mirror the same public or
private header path too. Integration examples live outside that unit-test tree.
Nesting is a folder, never a name glued with `_` (`command/def.c`, not
`command_def.c`). Each source without its mirrored header is reported; 0 is required.
Platform backends (`.../windows/...`, `.../linux/...`, `.../posix/...`) implement the
header of the folder above them and are checked against it. A private part may
have its header beside it in the source tree instead.
"""

import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TREES = (
    ('lib/lh/src/lh', 'lib/lh/include/lh'),
    ('src/authum/api', 'include/authum'),
    ('tests/src/authum', 'include/authum'),
)
BACKENDS = ('win', 'windows', 'linux', 'posix', 'unix', 'darwin', 'native')
SOURCES = ('.c', '.cpp')


def candidates(rel):
    """The headers a source at @p rel (no extension) may implement."""
    yield rel
    parts = rel.split('/')
    for i, part in enumerate(parts[:-1]):
        if part in BACKENDS:
            rest = parts[:i] + parts[i + 1:]
            yield '/'.join(rest)
            if len(rest) > i:
                yield '/'.join(rest[:i]) if i else ''


def main():
    missing = []
    for src, inc in TREES:
        base = os.path.join(ROOT, src)
        for d, _, files in os.walk(base):
            for f in sorted(files):
                stem, ext = os.path.splitext(f)
                if ext not in SOURCES:
                    continue
                rel = os.path.relpath(os.path.join(d, stem), base).replace(os.sep, '/')
                # A private part may have its header beside it, in the source tree.
                if not any(c and (os.path.exists(os.path.join(ROOT, inc, c + '.h')) or
                                  os.path.exists(os.path.join(base, c + '.h')) or
                                  (src == 'tests/src/authum' and
                                   os.path.exists(os.path.join(ROOT, 'src/authum', c + '.h'))))
                           for c in candidates(rel)):
                    missing.append(f'{src}/{rel}{ext}: no {inc}/{rel}.h')
    for line in missing:
        print(line)
    print(f'{len(missing)} source(s) without a mirrored header.')
    return 1 if missing else 0


if __name__ == '__main__':
    sys.exit(main())

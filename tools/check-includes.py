#!/usr/bin/env python3
"""Find #include lines that another include of the same file already brings in.

The owner's rule: bricks are built from smaller to larger, and a file includes
an entity once — when one of its includes already pulls a header in, the file
does not include that header again.

Only unconditional includes count as "already pulled in": an include inside
#if / #ifdef (other than the include guard) may be off in another build
configuration, so it never makes a direct include redundant.

    python tools/check-includes.py          # report, exit 1 when anything is found
    python tools/check-includes.py --fix    # delete the redundant include lines

A source file's own header (`x/y.c` including `<.../x/y.h>`) is kept: it is
the declaration the file implements. Line endings are kept.
"""

import os
import re
import sys

INCLUDE_DIRS = ('lib/lh/include', 'lib/lh/src', 'include', 'src')
SCAN = ('lib/lh/include', 'lib/lh/src', 'include', 'src', 'tests')
SKIP = ('lib/sqlite', 'lib/monocypher', 'lib/hlsdk', '/build', 'third_party')
INCLUDE = re.compile(r'^\s*#\s*include\s*<([^>]+)>')
OPEN = re.compile(r'^\s*#\s*(if|ifdef|ifndef)\b')
CLOSE = re.compile(r'^\s*#\s*endif\b')
GUARD = re.compile(r'^\s*#\s*ifndef\s+\w+_H\b')

_cache = {}


def resolve(name):
    """Path of header @p name, or None for a system or generated header."""
    for root in INCLUDE_DIRS:
        path = os.path.join(root, name)
        if os.path.isfile(path):
            return path.replace(os.sep, '/')
    return None


def direct(path):
    """(unconditional include names, [(line index, name, unconditional)]) of @p path."""
    if path in _cache:
        return _cache[path]
    lines = open(path, encoding='utf-8', errors='replace').read().replace('\r', '').split('\n')
    depth = 0
    guard_seen = False
    always = []
    every = []
    for index, line in enumerate(lines):
        if OPEN.match(line):
            if not guard_seen and GUARD.match(line):
                guard_seen = True
                continue
            depth += 1
            continue
        if CLOSE.match(line):
            depth = max(0, depth - 1)
            continue
        match = INCLUDE.match(line)
        if match:
            every.append((index, match.group(1), depth == 0))
            if depth == 0:
                always.append(match.group(1))
    _cache[path] = (always, every)
    return _cache[path]


def closure(name, seen):
    """Add @p name and every header it always brings in to @p seen."""
    path = resolve(name)
    if path is None or name in seen:
        return
    seen.add(name)
    for child in direct(path)[0]:
        closure(child, seen)


def own_header(path, name):
    """Whether @p name is the header the source file @p path implements."""
    return path.endswith(('.c', '.cpp')) and os.path.splitext(path)[0].endswith(
        os.path.splitext(name)[0])


def redundant_in(path):
    """(line index, name) of every include of @p path another of its includes brings in."""
    _, every = direct(path)
    result = []
    for index, name, unconditional in every:
        if not unconditional or resolve(name) is None or own_header(path, name):
            continue
        others = set()
        for _, other, other_unconditional in every:
            if other != name and other_unconditional:
                closure(other, others)
        if name in others:
            result.append((index, name))
    return result


def remove_line(path, index):
    raw = open(path, 'rb').read().decode('utf-8')
    newline = '\r\n' if '\r\n' in raw else '\n'
    lines = raw.replace('\r\n', '\n').split('\n')
    del lines[index]
    open(path, 'wb').write('\n'.join(lines).replace('\n', newline).encode('utf-8'))
    _cache.clear()


def main():
    fix = '--fix' in sys.argv[1:]
    total = 0
    for root in SCAN:
        for directory, _, names in os.walk(root):
            if any(s in directory.replace(os.sep, '/') + '/' for s in SKIP):
                continue
            for file_name in names:
                if not file_name.endswith(('.c', '.h', '.cpp')):
                    continue
                path = os.path.join(directory, file_name).replace(os.sep, '/')
                found = redundant_in(path)
                if not fix:
                    for index, name in found:
                        print(f'{path}:{index + 1}: <{name}> already comes in through another include')
                    total += len(found)
                    continue
                # One at a time: two includes that pull each other in must not both go.
                while found:
                    remove_line(path, found[0][0])
                    total += 1
                    found = redundant_in(path)
    print(f'{total} redundant include(s){" removed" if fix else ""}.')
    return 0 if fix or not total else 1


if __name__ == '__main__':
    sys.exit(main())

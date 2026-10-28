#!/usr/bin/env python3
"""Check that extern "C" wraps the functions of a header and nothing else.

`extern "C"` gives linkage to functions. Function declarations, inline
function definitions and function-type typedefs (a C++ function type carries
its language linkage) go inside the block; structs, enums, plain typedefs and
macros have no linkage and go outside.

    python tools/check-extern-c.py          # report, exit 1 when anything is wrong
    python tools/check-extern-c.py --fix    # rebuild the blocks of every header

Only headers that already use LH_COMPILER_EXTERN_C_* / AUTHUM_EXTERN_C_* are
rebuilt. The fix removes the markers and wraps each run of functions in one
BEGIN / END pair; declarations keep their order. A conditional (#if) that
mixes types and functions is reported for a hand fix. Line endings are kept.
"""

import importlib.util
import os
import re
import sys

SCAN = ('lib/lh/include', 'lib/lh/src', 'include', 'src')
SKIP = ('sqlite', 'monocypher', 'hlsdk', '/build', 'src/authum/gamedll')  # gamedll is C++
PAIRS = (('LH_COMPILER_EXTERN_C_BEGIN', 'LH_COMPILER_EXTERN_C_END'),
         ('AUTHUM_EXTERN_C_BEGIN', 'AUTHUM_EXTERN_C_END'))
DATA_START = re.compile(r'^(typedef\b|struct \w+\s*(\{.*)?$|enum\b|union \w+)')
COND_OPEN = re.compile(r'^\s*#\s*(if|ifdef|ifndef)\b')
COND_CLOSE = re.compile(r'^\s*#\s*endif\b')


def code_part(line):
    """@p line without a trailing comment."""
    line = re.sub(r'/\*.*?\*/', '', line)
    return re.sub(r'//.*$', '', line).rstrip()


def declaration_end(lines, i, end):
    """Index of the last line of the declaration or definition starting at @p i."""
    depth = 0
    opened = False
    j = i
    while j < end:
        text = code_part(lines[j])
        opened = opened or '{' in text
        depth += text.count('{') - text.count('}')
        if depth <= 0 and (text.endswith(';') or (opened and text.endswith('}'))):
            return j
        j += 1
    return end - 1


def classify(text):
    head = text.lstrip().split('\n', 1)[0]
    # Forward declarations, constant objects and static assertions have no C linkage to keep.
    if re.match(r'^(struct|union|enum) \w+;', head) or head.startswith(('static ', 'lh_assert_static')):
        return 'data'
    if DATA_START.match(head):
        # `typedef R (name)(...)` or `typedef R (*name)(...)`; not `__attribute__((...))`.
        is_function_type = (head.startswith('typedef') and '{' not in text and
                            re.search(r'\(\s*\*?\s*\w+\s*\)\s*\(', code_part(text)) is not None)
        return 'code' if is_function_type else 'data'
    return 'code'


def items(lines, begin, end):
    """Split lines [begin, end) into [(kind, first, last)]: data, code, blank, mixed."""
    result = []
    i = begin
    while i < end:
        line = lines[i]
        if not line.strip():
            result.append(('blank', i, i))
            i += 1
            continue
        first = i
        if line.lstrip().startswith(('/*', '//')):
            while line.lstrip().startswith('/*') and '*/' not in lines[i]:
                i += 1
            nxt = lines[i + 1] if i + 1 < end else ''
            if nxt.strip() and not nxt.lstrip().startswith(('/*', '//')):
                i += 1  # the comment documents the next declaration
            else:
                result.append(('comment', first, i))
                i += 1
                continue
        head = lines[i]
        if head.lstrip().startswith('#'):
            if COND_OPEN.match(head):
                depth = 0
                k = i
                inner = []
                while k < end:
                    if COND_OPEN.match(lines[k]):
                        depth += 1
                    elif COND_CLOSE.match(lines[k]):
                        depth -= 1
                    if depth == 0:
                        break
                    k += 1
                kinds = {kind for kind, _, _ in items(lines, i + 1, k)
                         if kind in ('data', 'code', 'mixed')}
                kind = 'mixed' if len(kinds) > 1 or 'mixed' in kinds else (kinds.pop() if kinds else 'data')
                result.append((kind, first, k))
                i = k + 1
                continue
            j = i
            while lines[j].rstrip().endswith('\\') and j + 1 < end:
                j += 1
            result.append(('data', first, j))  # a macro or another directive
            i = j + 1
            continue
        j = declaration_end(lines, i, end)
        result.append((classify('\n'.join(lines[i:j + 1])), first, j))
        i = j + 1
    return result


def body(lines):
    """[begin, end) of the declarations: after the top includes, before the guard's #endif."""
    guard_end = max(i for i, line in enumerate(lines) if COND_CLOSE.match(line))
    begin = 0
    for i, line in enumerate(lines[:guard_end]):
        if line.startswith('#include') or re.match(r'#define \w+_H\b', line):
            begin = i + 1
        elif line.strip() and not line.startswith('#') and not line.lstrip().startswith(('/*', '*')):
            break
    return begin, guard_end


def branches(lines, first, last):
    """Directive lines and [begin, end) branches of the conditional in [first, last]."""
    marks = [first]
    depth = 0
    for k in range(first, last + 1):
        if COND_OPEN.match(lines[k]):
            depth += 1
        elif COND_CLOSE.match(lines[k]):
            depth -= 1
        elif depth == 1 and re.match(r'^\s*#\s*(else|elif)\b', lines[k]):
            marks.append(k)
    marks.append(last)
    return marks


def emit(plain, begin, end, begin_mark, end_mark):
    """Lines [begin, end) with each run of functions wrapped; the block is closed at the end."""
    out = []
    inside = False
    for kind, first, last in items(plain, begin, end):
        if kind == 'mixed':
            # Close the block, then wrap each branch of the conditional on its own.
            if inside:
                while out and not out[-1].strip():
                    out.pop()
                out += ['', end_mark, '']
                inside = False
            marks = branches(plain, first, last)
            for a, b in zip(marks, marks[1:]):
                out.append(plain[a])
                inner = emit(plain, a + 1, b, begin_mark, end_mark)
                if inner and inner[0].strip():
                    out.append('')
                out += inner
                if out and out[-1].strip():
                    out.append('')
            out.append(plain[last])
            continue
        if kind == 'data' and inside:
            while out and not out[-1].strip():
                out.pop()
            out += ['', end_mark, '']
            inside = False
        elif kind == 'code' and not inside:
            while out and not out[-1].strip():
                out.pop()
            out += ['', begin_mark, '']
            inside = True
        if kind == 'blank' and out and not out[-1].strip():
            continue
        out += plain[first:last + 1]
    while out and not out[-1].strip():
        out.pop()
    if inside:
        out += ['', end_mark]
    return out


def rebuild(lines, begin_mark, end_mark):
    """Lines with the extern "C" markers rebuilt around exactly the functions."""
    plain = [line for line in lines if line.strip() not in (begin_mark, end_mark)]
    begin, end = body(plain)
    out = plain[:begin] + emit(plain, begin, end, begin_mark, end_mark) + [''] + plain[end:]
    return re.sub(r'\n{3,}', '\n\n', '\n'.join(out)).split('\n')


def with_marker_header(lines, path):
    """@p lines with the header that defines the markers, unless an include already brings it."""
    spec = importlib.util.spec_from_file_location(
        'check_includes', os.path.join(os.path.dirname(os.path.abspath(__file__)), 'check-includes.py'))
    includes = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(includes)
    needed = 'lh/compiler/extern/c.h' if path.startswith('lib/lh/') else 'authum/extern.h'
    seen = set()
    for line in lines:
        match = includes.INCLUDE.match(line)
        if match:
            includes.closure(match.group(1), seen)
    if needed in seen:
        return lines
    last = max(i for i, line in enumerate(lines) if line.startswith('#include'))
    return lines[:last + 1] + [f'#include <{needed}>'] + lines[last + 1:]


def declares_function(lines):
    """Whether the header declares or defines a function (name at column 0, return type above)."""
    for i, line in enumerate(lines[1:], 1):
        above = code_part(lines[i - 1]).strip()
        if line.startswith('lh_assert_static'):
            continue
        if (re.match(r'^[a-z_]\w*\s*\(', line) and above and not above.startswith('#')
                and not above.endswith((';', '{', '}', ',', '\\'))):
            return True
    return False


def main():
    fix = '--fix' in sys.argv[1:]
    wrong = []
    manual = []
    for root in SCAN:
        for directory, _, names in os.walk(root):
            if any(s in directory.replace(os.sep, '/') for s in SKIP):
                continue
            for name in names:
                if not name.endswith('.h'):
                    continue
                path = os.path.join(directory, name).replace(os.sep, '/')
                raw = open(path, 'rb').read().decode('utf-8')
                newline = '\r\n' if '\r\n' in raw else '\n'
                lines = raw.replace('\r\n', '\n').split('\n')
                if not any(line.strip() in pair for line in lines for pair in PAIRS) and \
                        declares_function(lines):
                    # A header with functions and no block at all: give it one.
                    pair = PAIRS[0] if path.startswith('lib/lh/') else PAIRS[1]
                    new = rebuild(lines, *pair)
                    if new != lines:
                        wrong.append(path)
                        if fix:
                            new = with_marker_header(new, path)
                            open(path, 'wb').write('\n'.join(new).replace('\n', newline).encode('utf-8'))
                    continue
                for begin_mark, end_mark in PAIRS:
                    if not any(line.strip() in (begin_mark, end_mark) for line in lines):
                        continue
                    new = rebuild(lines, begin_mark, end_mark)
                    if new is None:
                        manual.append(path)
                        continue
                    if new != lines:
                        wrong.append(path)
                        if fix:
                            open(path, 'wb').write('\n'.join(new).replace('\n', newline).encode('utf-8'))
                            lines = new
    for path in wrong:
        print(f'{path}: extern "C" does not wrap exactly its functions{" (fixed)" if fix else ""}')
    for path in manual:
        print(f'{path}: a conditional mixes types and functions; fix by hand')
    print(f'{len(wrong)} header(s){" fixed" if fix else " to fix"}; {len(manual)} by hand.')
    return 0 if (fix or not wrong) and not manual else 1


if __name__ == '__main__':
    sys.exit(main())

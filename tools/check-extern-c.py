#!/usr/bin/env python3
"""Find data types declared inside an extern "C" block.

`extern "C"` gives linkage to functions; a struct, an enum or a plain typedef
has none, so it is declared outside the block. Function declarations and
function-type typedefs (a C++ function type carries its language linkage)
stay inside.

    python tools/check-extern-c.py          # report, exit 1 when anything is found
    python tools/check-extern-c.py --fix    # close the block around the data types

The fix keeps every declaration in its place: it ends the block before a run
of data types and opens it again before the next function, then drops blocks
left empty. A region with a preprocessor conditional is reported, not fixed.
Line endings are kept.
"""

import os
import re
import sys

SCAN = ('lib/lh/include', 'lib/lh/src', 'include', 'src')
SKIP = ('sqlite', 'monocypher', 'hlsdk', '/build')
PAIRS = (('LH_COMPILER_EXTERN_C_BEGIN', 'LH_COMPILER_EXTERN_C_END'),
         ('AUTHUM_EXTERN_C_BEGIN', 'AUTHUM_EXTERN_C_END'))
DATA_START = re.compile(r'^(typedef\b|struct \w+\s*(\{.*)?$|enum\b|union \w+)')


def items(lines, begin, end):
    """Split lines (begin, end) into [(kind, first, last)]; kind is data / code / blank."""
    result = []
    i = begin
    while i < end:
        line = lines[i]
        if not line.strip():
            result.append(('blank', i, i))
            i += 1
            continue
        first = i
        # A comment belongs to the declaration after it.
        if line.lstrip().startswith('/*'):
            while '*/' not in lines[i]:
                i += 1
            if i + 1 < end and lines[i + 1].strip() and not lines[i + 1].lstrip().startswith('/*'):
                i += 1
            else:
                result.append(('code', first, i))
                i += 1
                continue
        head = lines[i]
        j = i
        if head.lstrip().startswith('#'):
            # A directive with its continuation lines. A macro has no linkage,
            # like a type; a conditional cannot be split safely.
            while lines[j].rstrip().endswith('\\') and j + 1 < end:
                j += 1
            if re.match(r'^\s*#\s*(if|ifdef|ifndef)\b', head):
                # A conditional that only chooses macros is data as a whole.
                depth = 0
                only_directives = True
                k = i
                while k < end:
                    stripped = lines[k].strip()
                    if re.match(r'#\s*(if|ifdef|ifndef)\b', stripped):
                        depth += 1
                    elif re.match(r'#\s*endif\b', stripped):
                        depth -= 1
                    elif stripped and not stripped.startswith(('#', '/*', '*', '//')) and \
                            not lines[k - 1].rstrip().endswith('\\'):
                        only_directives = False
                    if depth == 0:
                        break
                    k += 1
                result.append(('data' if only_directives and depth == 0 else 'pre', first, k))
                i = k + 1
                continue
            conditional = re.match(r'^\s*#\s*(elif|else|endif)\b', head)
            result.append(('pre' if conditional else 'data', first, j))
            i = j + 1
            continue
        depth = 0
        while j < end:
            depth += lines[j].count('{') - lines[j].count('}')
            if depth <= 0 and lines[j].rstrip().endswith(';'):
                break
            j += 1
        text = '\n'.join(lines[i:j + 1])
        kind = 'code'
        if DATA_START.match(head):
            is_function_type = (head.startswith('typedef') and '{' not in text
                                and '(' in text)
            kind = 'code' if is_function_type else 'data'
        result.append((kind, first, j))
        i = j + 1
    return result


def regions(lines):
    for begin_mark, end_mark in PAIRS:
        i = 0
        while i < len(lines):
            if lines[i].strip() == begin_mark:
                j = i + 1
                while j < len(lines) and lines[j].strip() != end_mark:
                    j += 1
                yield begin_mark, end_mark, i, j
                i = j
            i += 1


def rewrite(lines):
    """Return (new lines, data types moved, regions skipped)."""
    for begin_mark, end_mark, begin, end in regions(lines):
        parts = items(lines, begin + 1, end)
        if not any(kind == 'data' for kind, _, _ in parts):
            continue
        if any(kind == 'pre' for kind, _, _ in parts):
            return lines, 0, 1
        out = lines[:begin]
        inside = False
        moved = 0
        for kind, first, last in parts:
            chunk = lines[first:last + 1]
            if kind == 'data' and inside:
                while out and not out[-1].strip():
                    out.pop()
                out += ['', end_mark, '']
                inside = False
            if kind == 'code' and not inside:
                while out and not out[-1].strip():
                    out.pop()
                out += ['', begin_mark, '']
                inside = True
            if kind == 'data':
                moved += 1
            if kind == 'blank' and out and not out[-1].strip():
                continue
            out += chunk
        while out and not out[-1].strip():
            out.pop()
        if inside:
            out += ['', end_mark]
        rest = lines[end + 1:]
        new = out + rest
        again, more, skipped = rewrite(new)
        return again, moved + more, skipped
    return lines, 0, 0


def main():
    fix = '--fix' in sys.argv[1:]
    total = 0
    skipped = []
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
                new, moved, skip = rewrite(lines)
                if skip:
                    skipped.append(path)
                if not moved:
                    continue
                total += moved
                print(f'{path}: {moved} data type(s) inside extern "C"')
                if fix:
                    text = '\n'.join(new)
                    text = re.sub(r'\n{3,}', '\n\n', text)
                    open(path, 'wb').write(text.replace('\n', newline).encode('utf-8'))
    for path in skipped:
        print(f'{path}: has a conditional inside extern "C"; fix by hand')
    print(f'{total} data type(s){" moved out" if fix else ""}; {len(skipped)} file(s) to fix by hand.')
    return 0 if (fix or not total) and not skipped else 1


if __name__ == '__main__':
    sys.exit(main())

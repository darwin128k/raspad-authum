#!/usr/bin/env python3
"""Find functions that touch a struct field without being that field's accessor.

The rule (lib/lh/AGENTS.md 0.0): only a struct's own level-1 getters and
setters name a field; everything else calls them.

    python tools/check-fields.py <file.c>...    # every field access outside accessors

An accessor is a function whose name ends in `_get_<field>`, `_set_<field>`,
`_get_<field>_mut` or that is a plain `get_`/`set_` of the struct. Accesses
are `ptr->field` and `value.field` on code lines (comments and strings are
skipped). Initializer macros (`{.field = ...}`) are not function code.
"""

import re
import sys

ACCESS = re.compile(r'(?:\b(\w+)\s*->\s*|\b([a-z_]\w*)\s*\.\s*)([a-z_]\w*)\b(?!\s*\()')
STRIP = re.compile(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', re.S)


def accesses(path):
    text = open(path, encoding='utf-8', errors='replace').read().replace('\r', '')
    code = STRIP.sub(lambda m: re.sub(r'[^\n]', ' ', m.group(0)), text)
    found = []
    for m in re.finditer(r'^([a-z_]\w*)\((?:[^;{]|\n)*?\)\n\{\n(.*?)\n\}\n', code, re.S | re.M):
        name, body = m.group(1), m.group(2)
        line0 = code.count('\n', 0, m.start(2)) + 1
        for a in ACCESS.finditer(body):
            field = a.group(3)
            base = a.group(1) or a.group(2)
            if base and base[0].isdigit():
                continue
            single = re.escape(field[:-1]) if field.endswith('s') else re.escape(field)
            accessor = (re.search(r'_(get|set|init)_(' + re.escape(field) + '|' + single + r')(_mut|_v|_fn|_str|_word)?$', name) or
                        re.search(r'_is_' + re.escape(field) + r'(_\w+)?$', name))
            if accessor:
                continue
            found.append((line0 + body.count('\n', 0, a.start()), name, base, field))
    return found


def own_fields():
    """Field names of the structs LH and Authum declare (fields macros and struct bodies)."""
    import os
    names = set()
    for root in ('lib/lh/include', 'lib/lh/src', 'include', 'src'):
        for directory, _, files in os.walk(root):
            if any(s in directory.replace(os.sep, '/') for s in ('sqlite', 'monocypher', 'hlsdk')):
                continue
            for f in files:
                if not f.endswith('.h'):
                    continue
                text = STRIP.sub(' ', open(os.path.join(directory, f), encoding='utf-8',
                                           errors='replace').read().replace('\r', ''))
                for body in re.findall(r'#define\s+\w+_fields\([^)]*\)((?:[^\n]*\\\n)*[^\n]*)', text):
                    names.update(re.findall(r'(\w+)\s*(?:\[[^\]]*\])?\s*;?\s*\\?\s*(?:\n|$)', body))
                for body in re.findall(r'\bstruct\s+(?:lh|authum)_\w+\s*\{(.*?)\}', text, re.S):
                    names.update(re.findall(r'(\w+)\s*(?:\[[^\]]*\])?\s*;', body))
    return {n for n in names if not n.isupper() and not n.startswith('lh_')}


def main():
    total = 0
    paths = [a for a in sys.argv[1:] if a != '--own']
    own = own_fields() if '--own' in sys.argv else None
    for path in paths:
        for line, name, base, field in accesses(path):
            if own is not None and field not in own:
                continue
            print(f'{path}:{line}: {name}: {base}{"->" if True else "."}{field}')
            total += 1
    print(f'{total} raw field access(es).')
    return 1 if total else 0


if __name__ == '__main__':
    sys.exit(main())

#!/usr/bin/env python3
"""List the fields of our structs and which level-1 accessors they lack (lib/lh/AGENTS.md 0.0).

    python tools/list-fields.py <include-dir> [<prefix-filter>]
    python tools/list-fields.py lib/lh/include lh_ui_

For every `struct NAME { ... };` with a `typedef struct NAME NAME_t;` it reads
the members, also through `*_fields(...)` macros (nested ones too), and prints
one line per field that lacks an accessor: `<struct_t> <field> <type> <kind>
<missing accessors>`. The kind is the one ::tools/gen-accessors.py takes:
`flag` for an lh_bool_t, `owned` for an array, `view` for a struct held by
value, `fn` for a callback, a value otherwise. ::structs is what
::tools/use-accessors.py imports.
"""

import os
import re
import sys

STRUCT = re.compile(r'^struct (\w+)\s*\{(.*?)^\};', re.M | re.S)
TYPEDEF_STRUCT = re.compile(r'^typedef struct(?: \w+)?\s*\{(.*?)^\}\s*(\w+_t)\s*;', re.M | re.S)
MACRO = re.compile(r'^#\s*define (\w+_fields)(?:\(([^)]*)\))?((?:[^\n]*\\\n)*[^\n]*)', re.M)
MEMBER = re.compile(r'^\s*((?:const\s+)?(?:struct\s+)?[\w ]+?[\s*]+)(\w+)(\[[^\]]*\])?\s*;', re.M)


def read_all(roots):
    texts = {}
    for root in roots:
        for d, _, files in os.walk(root):
            for f in files:
                if f.endswith('.h') or f.endswith('.c'):
                    p = os.path.join(d, f)
                    texts[p] = open(p, encoding='utf-8', errors='replace').read().replace('\r', '')
    return texts


def strip_comments(text):
    return re.sub(r'/\*.*?\*/|//[^\n]*', '', text, flags=re.S)


def expand(body, macros, depth=0):
    """Replace every `X_fields(args);` in @p body by the members it stands for."""
    if depth > 8:
        return body

    def one(call):
        name = call.group(1)
        if name not in macros:
            return call.group(0)
        params, mbody = macros[name]
        values = split_args(call.group(2)) if call.group(2) is not None else []
        for p, v in zip(params, values):
            mbody = re.sub(r'\b' + re.escape(p) + r'\b', v, mbody)
        return expand(mbody, macros, depth + 1) + ';'
    return re.sub(r'\b(\w+_fields)\s*(?:\(((?:[^()]|\([^()]*\))*)\))?\s*;', one, body)


def split_args(text):
    out, depth, cur = [], 0, ''
    for ch in text:
        if ch == ',' and depth == 0:
            out.append(cur.strip())
            cur = ''
            continue
        depth += ch == '('
        depth -= ch == ')'
        cur += ch
    if cur.strip():
        out.append(cur.strip())
    return out


def structs(roots):
    """{struct_t: {'prefix', 'path', 'fields': [(field, ctype, array, kind)]}} for our structs."""
    texts = read_all(roots)
    every = '\n'.join(texts.values())
    macros = {}
    for m in MACRO.finditer(every):
        params = (m.group(2) or '').replace('\\\n', ' ')
        macros[m.group(1)] = ([p.strip() for p in params.split(',') if p.strip()],
                              strip_comments(m.group(3).replace('\\\n', '\n')))
    enum_types = set(re.findall(r'typedef enum[^;]*?\}\s*(\w+_t);', every, re.S))
    struct_types = set(re.findall(r'typedef struct \w+ (\w+_t);', every))
    struct_types |= set(re.findall(r'typedef struct(?: \w+)?\s*\{[^}]*\}\s*(\w+_t);', every, re.S))
    struct_types -= enum_types
    # `typedef lh_math_rect_t lh_ui_rect_t;`: an alias of a struct is a struct.
    aliases = re.findall(r'typedef (\w+_t) (\w+_t);', every)
    for _ in range(4):
        struct_types |= {new for old, new in aliases if old in struct_types}
    out = {}
    for path, text in sorted(texts.items()):
        bodies = []
        for m in STRUCT.finditer(text):
            tm = re.search(r'typedef struct ' + m.group(1) + r' (\w+_t);', text)
            if tm:
                bodies.append((tm.group(1), m.group(2)))
        # `typedef struct NAME { ... } NAME_t;` in one
        for m in TYPEDEF_STRUCT.finditer(text):
            bodies.append((m.group(2), m.group(1)))
        for tname, raw_body in bodies:
            body = expand(strip_comments(raw_body), macros)
            fields = []
            for mm in MEMBER.finditer(body):
                ctype = ' '.join(mm.group(1).split()).replace(' *', '*').replace('*', ' *').strip()
                field, array = mm.group(2), mm.group(3)
                bare = ctype.replace('const ', '').replace('struct ', '').strip()
                if array:
                    kind = 'array'
                elif bare == 'lh_bool_t':
                    kind = 'flag'
                elif bare.endswith('_cb') or (bare.endswith('_fn *') and '*' in bare):
                    kind = 'fn'
                elif '*' not in bare and bare in struct_types:
                    kind = 'view'
                else:
                    kind = ''
                fields.append((field, ctype, bool(array), kind))
            out[tname] = {'prefix': tname[:-1], 'path': path, 'fields': fields}
    return out, set(re.findall(r'^(lh_\w+|authum_\w+)\(', every, re.M))


def needed(prefix, field, kind):
    if kind in ('owned', 'array'):
        return [f'{prefix}get_{field}', f'{prefix}get_{field}_mut']
    if kind == 'view':
        return [f'{prefix}get_{field}', f'{prefix}get_{field}_mut', f'{prefix}set_{field}']
    if kind == 'fn':
        return [f'{prefix}get_{field}_fn', f'{prefix}set_{field}_fn']
    if kind == 'flag':
        return [f'{prefix}is_{field}', f'{prefix}set_{field}']
    return [f'{prefix}get_{field}', f'{prefix}set_{field}']


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    root = args[0]
    want = args[1] if len(args) > 1 else ''
    found, declared = structs([root])
    for tname, info in found.items():
        if not tname.startswith(want):
            continue
        for field, ctype, array, kind in info['fields']:
            missing = [n[len(info['prefix']):] for n in needed(info['prefix'], field, kind)
                       if n not in declared]
            if missing:
                print(tname, field, ctype.replace(' ', ''), kind or 'value', ','.join(missing),
                      os.path.relpath(info['path']))
    return 0


if __name__ == '__main__':
    sys.exit(main())

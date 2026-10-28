#!/usr/bin/env python3
"""Generate the level-1 getters and setters of a struct (lib/lh/AGENTS.md 0.0).

    python tools/gen-accessors.py <header.h> <source.c> <struct_t> <prefix> <field>:<type>[:view] ...

For each field, `<prefix>get_<field>` and `<prefix>set_<field>` are declared
in the header (inside its extern "C" block, created when missing) and defined
in the source (created when missing). A `:view` field is a small struct held
by value: its getter returns a const pointer, its setter takes it by value.
Accessors that already exist are left alone. Line endings are kept.
"""

import os
import re
import sys


def read(path, like=None):
    if os.path.exists(path):
        raw = open(path, 'rb').read().decode('utf-8')
        return raw.replace('\r\n', '\n'), ('\r\n' if '\r\n' in raw else '\n')
    return None, '\n'


def write(path, text, newline):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    text = re.sub(r'\n{3,}', '\n\n', text)
    open(path, 'wb').write(text.replace('\n', newline).encode('utf-8'))


def main():
    header, source, struct, prefix = sys.argv[1:5]
    fields = [f.split(':') for f in sys.argv[5:]]
    htext, hnl = read(header)
    stext, snl = read(source)
    marker_begin, marker_end = (('LH_COMPILER_EXTERN_C_BEGIN', 'LH_COMPILER_EXTERN_C_END')
                                if '/lib/lh/' in os.path.abspath(header).replace(os.sep, '/') else
                                ('AUTHUM_EXTERN_C_BEGIN', 'AUTHUM_EXTERN_C_END'))
    decls = []
    defs = []
    for spec in fields:
        field, ctype = spec[0], spec[1]
        view = len(spec) > 2 and spec[2] == 'view'
        get, set_ = f'{prefix}get_{field}', f'{prefix}set_{field}'
        ret = f'const {ctype} *' if view else ctype
        if not re.search(r'^' + get + r'\(', htext, re.M):
            sep = '' if ret.endswith('*') else ' '
            decls.append(f'/** @brief The @c {field} of @p self. */\n{ret}\n{get}(const {struct} *self);\n')
            body = f'return lh_addr_of(self->{field});' if view else f'return self->{field};'
            defs.append(f'{ret}\n{get}(const {struct} *self)\n{{\n    lh_assert_runtime_ref(self);\n'
                        f'    {body}\n}}\n')
        if not re.search(r'^' + set_ + r'\(', htext, re.M):
            decls.append(f'/** @brief Store @p {field} as the @c {field} of @p self. */\nlh_void\n'
                         f'{set_}({struct} *self, {ctype} {field});\n')
            defs.append(f'lh_void\n{set_}({struct} *self, {ctype} {field})\n{{\n'
                        f'    lh_assert_runtime_ref(self);\n    self->{field} = {field};\n}}\n')
    if not decls:
        print('nothing to add')
        return 0
    block = '\n'.join(decls)
    if marker_end in htext:
        i = htext.rindex(marker_end)
        htext = htext[:i] + block + '\n' + htext[i:]
    else:
        i = htext.rindex('#endif')
        htext = htext[:i] + f'{marker_begin}\n\n{block}\n{marker_end}\n\n' + htext[i:]
    for inc in ('#include <lh/compiler/extern/c.h>', '#include <lh/void.h>'):
        if '/lib/lh/' in os.path.abspath(header).replace(os.sep, '/') and inc not in htext:
            last = max(m.end() for m in re.finditer(r'^#include[^\n]*\n', htext, re.M))
            htext = htext[:last] + inc + '\n' + htext[last:]
    write(header, htext, hnl)
    if stext is None:
        name = re.sub(r'^(?:.*/)?include/', '', header.replace(os.sep, '/'), count=1)
        stext, snl = f'#include <{name}>\n#include <lh/assert.h>\n\n', hnl
    elif '#include <lh/assert.h>' not in stext:
        stext = stext.replace('\n\n', '\n#include <lh/assert.h>\n\n', 1)
    write(source, stext.rstrip('\n') + '\n\n' + '\n'.join(defs), snl)
    print(f'{struct}: {len(decls)} accessor(s) added')
    return 0


if __name__ == '__main__':
    sys.exit(main())

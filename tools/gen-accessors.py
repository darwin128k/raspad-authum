#!/usr/bin/env python3
"""Generate the level-1 getters and setters of a struct (lib/lh/AGENTS.md 0.0).

    python tools/gen-accessors.py <header.h> <source.c> <struct_t> <prefix> <field>:<type>[:kind][=<meaning>] ...
    python tools/gen-accessors.py ... --owner <noun> ...

Kinds:
  (none)  a value: `<prefix>get_<field>` returns it, `<prefix>set_<field>` stores it;
  flag    a boolean: `<prefix>is_<field>` / `<prefix>set_<field>`;
  view    a small struct held by value: the getter returns a const pointer, the
          setter takes it by value;
  owned   a member that owns memory (a string, an array): `get_<field>` returns a
          const pointer, `get_<field>_mut` a mutable one; no setter;
  fn      a function pointer whose plain names are taken by call wrappers:
          `get_<field>_fn` / `set_<field>_fn`.

<meaning> is a noun phrase ("the native window handle") the full briefs are
written from; `--owner` names what @p self is ("window"). A field without a
meaning gets a placeholder brief, and the tool says so: rewrite it.

Declarations go into the header's extern "C" block (created when missing),
definitions into the source (created when missing). Existing accessors are
left alone. Line endings are kept. Run `tools/check-includes.py --fix` after.
"""

import os
import re
import sys


def read(path):
    if os.path.exists(path):
        raw = open(path, 'rb').read().decode('utf-8')
        return raw.replace('\r\n', '\n'), ('\r\n' if '\r\n' in raw else '\n')
    return None, '\n'


def write(path, text, newline):
    os.makedirs(os.path.dirname(path) or '.', exist_ok=True)
    text = re.sub(r'\n{3,}', '\n\n', text)
    open(path, 'wb').write(text.replace('\n', newline).encode('utf-8'))


def declared(htext, name):
    return re.search(r'^' + re.escape(name) + r'\(', htext, re.M) is not None


def getter(struct, name, ret, body, mutable=False):
    const = '' if mutable else 'const '
    decl = f'{ret}\n{name}({const}{struct} *self);\n'
    definition = (f'{ret}\n{name}({const}{struct} *self)\n{{\n'
                  f'    lh_assert_runtime_ref(self);\n    {body}\n}}\n')
    return decl, definition


def setter(struct, name, ctype, field):
    decl = f'lh_void\n{name}({struct} *self, {ctype} {field});\n'
    definition = (f'lh_void\n{name}({struct} *self, {ctype} {field})\n{{\n'
                  f'    lh_assert_runtime_ref(self);\n    self->{field} = {field};\n}}\n')
    return decl, definition


def doc(lines):
    return '/**\n' + ''.join((' * ' + line).rstrip() + '\n' for line in lines) + ' */\n'


def cap(text):
    return text[0].upper() + text[1:]


def main():
    args = sys.argv[1:]
    owner = 'object'
    if '--owner' in args:
        i = args.index('--owner')
        owner = args[i + 1]
        del args[i:i + 2]
    header, source, struct, prefix = args[:4]
    fields = []
    for a in args[4:]:
        spec, _, meaning = a.partition('=')
        fields.append((spec.split(':'), meaning))
    htext, hnl = read(header)
    stext, snl = read(source)
    in_lh = '/lib/lh/' in os.path.abspath(header).replace(os.sep, '/')
    marker_begin, marker_end = (('LH_COMPILER_EXTERN_C_BEGIN', 'LH_COMPILER_EXTERN_C_END') if in_lh
                                else ('AUTHUM_EXTERN_C_BEGIN', 'AUTHUM_EXTERN_C_END'))
    decls, defs = [], []
    placeholders = []
    self_param = f'@param self The {owner}; checked by ::lh_assert_runtime_ref.'

    def add(lines, pair):
        decls.append(doc(lines) + pair[0])
        defs.append(pair[1])

    def get_doc(field, meaning, mutable=False):
        if not meaning:
            placeholders.append(field)
            return [f'@brief The @c {field} of @p self{", to change in place" if mutable else ""}.']
        if mutable:
            return [f'@brief {cap(meaning)}, to change in place (level 1).', '', self_param,
                    '@return The member itself, never ::lh_null.']
        return [f'@brief {cap(meaning)} (level 1).', '', self_param]

    def set_doc(field, meaning):
        if not meaning:
            placeholders.append(field)
            return [f'@brief Store @p {field} as the @c {field} of @p self.']
        return [f'@brief Store {meaning} (level 1).', '',
                'A level-1 setter: the value is stored as given and is not validated.', '',
                self_param, f'@param {field} The new value.']

    for spec, meaning in fields:
        field, ctype = spec[0], spec[1].replace('*', ' *').replace('  ', ' ').strip()
        kind = spec[2] if len(spec) > 2 else ''
        if kind == 'owned':
            get, mut = f'{prefix}get_{field}', f'{prefix}get_{field}_mut'
            if not declared(htext, get):
                add(get_doc(field, meaning),
                    getter(struct, get, f'const {ctype} *', f'return lh_addr_of(self->{field});'))
            if not declared(htext, mut):
                add(get_doc(field, meaning, True),
                    getter(struct, mut, f'{ctype} *', f'return lh_addr_of(self->{field});', True))
            continue
        suffix = '_fn' if kind == 'fn' else ''
        verb = 'is' if kind == 'flag' else 'get'
        get, set_ = f'{prefix}{verb}_{field}{suffix}', f'{prefix}set_{field}{suffix}'
        if not declared(htext, get):
            if kind == 'view':
                add(get_doc(field, meaning),
                    getter(struct, get, f'const {ctype} *', f'return lh_addr_of(self->{field});'))
            else:
                add(get_doc(field, meaning), getter(struct, get, ctype, f'return self->{field};'))
        if not declared(htext, set_):
            add(set_doc(field, meaning), setter(struct, set_, ctype, field))
    if not decls:
        print('nothing to add')
        return 0

    block = '\n'.join(decls)
    created = marker_end not in htext
    if not created:
        i = htext.rindex(marker_end)
        htext = htext[:i] + block + '\n' + htext[i:]
    else:
        i = htext.rindex('#endif')
        htext = htext[:i] + f'{marker_begin}\n\n{block}\n{marker_end}\n\n' + htext[i:]
    # A header that already had the block has what it needs; a new block needs its
    # marker and lh_void. Run `tools/check-includes.py --fix` afterwards either way.
    if in_lh and created:
        first_use = htext.index(marker_begin)
        for inc in ('#include <lh/compiler/extern/c.h>', '#include <lh/void.h>'):
            if inc not in htext:
                last = max(m.end() for m in re.finditer(r'^#include[^\n]*\n', htext[:first_use], re.M))
                htext = htext[:last] + inc + '\n' + htext[last:]
                first_use += len(inc) + 1
    write(header, htext, hnl)

    if stext is None:
        name = re.sub(r'^(?:.*/)?include/', '', header.replace(os.sep, '/'), count=1)
        stext, snl = f'#include <{name}>\n#include <lh/assert.h>\n\n', hnl
    elif '#include <lh/assert.h>' not in stext and '#include <lh/assert/runtime.h>' not in stext:
        stext = stext.replace('\n\n', '\n#include <lh/assert.h>\n\n', 1)
    write(source, stext.rstrip('\n') + '\n\n' + '\n'.join(defs), snl)
    print(f'{struct}: {len(decls)} accessor(s) added')
    if placeholders:
        print('placeholder briefs, rewrite them:', ' '.join(sorted(set(placeholders))))
    return 0


if __name__ == '__main__':
    sys.exit(main())

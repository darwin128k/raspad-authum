#!/usr/bin/env python3
"""Make C sources reach struct fields through their level-1 accessors (lib/lh/AGENTS.md 0.0).

    python tools/use-accessors.py [--dry] <file.c> ...

The struct types, their fields and their accessors come from the headers
(::tools/list-fields.py reads them). In each function the type of every
variable is taken from its declaration, and every member chain on one of ours
is rewritten from the left:

    p->f = x;          <prefix>set_f(p, x);
    p->f += x;  ++p->f; <prefix>set_f(p, <prefix>get_f(p) + x);
    p->f               <prefix>get_f(p)      (is_f for a flag, get_f_fn for a callback)
    p->f(...)          <prefix>get_f_fn(p)(...)
    p->s.g             <inner>get_g(<prefix>get_s(p))   (get_s_mut when the chain is written)
    lh_addr_of(p->s)   <prefix>get_s(p) / get_s_mut(p)
    p->a[i]            <prefix>get_a(p)[i] / get_a_mut(p)[i]

A function that is itself a level-1 accessor (one assert and one read or
write of a field) is left alone, and so is any chain inside sizeof. What is
left over is printed, one line per place, for a hand fix. Line endings are
kept.
"""

import importlib.util
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location('list_fields', os.path.join(HERE, 'list-fields.py'))
lf = importlib.util.module_from_spec(spec)
spec.loader.exec_module(lf)

SPLIT = re.compile(r'(\n[a-zA-Z_][^\n;{}()]*\n?[a-z_]\w*\((?:[^;{]|\n)*?\)\n\{\n.*?\n\}\n)', re.S)
LEVEL1 = re.compile(r'\{\n(?:    lh_assert_runtime_ref\(self\);\n)?'
                    r'    (?:return (?:lh_addr_of\()?self->\w+(?:\[\w+\])?\)?;|self->\w+(?:\[\w+\])? = \w+;)\n\}\n$')
DECL = re.compile(r'(?<![\w.>])(const\s+)?(?:struct\s+)?(lh_\w+?)(_t|_ptr|_cptr)?\b(\s*\*\s*|\s+)(?:const\s+)?(\w+)\s*(?=[,;=)\[])')
CHAIN = re.compile(r'(?<![\w.>])([A-Za-z_]\w*)((?:\s*(?:->|\.)\s*[A-Za-z_]\w*)+)')
SEG = re.compile(r'\s*(->|\.)\s*([A-Za-z_]\w*)')


class Model:
    def __init__(self, roots):
        self.structs, self.declared = lf.structs(roots)
        self.by_struct = {}
        for tname, info in self.structs.items():
            self.by_struct[tname] = {f: (ctype, kind) for f, ctype, array, kind in info['fields']}
        names = {}
        text = '\n'.join(lf.read_all(roots).values())
        for m in re.finditer(r'typedef struct (\w+) (\w+_t);', text):
            names[m.group(1)] = m.group(2)
        self.struct_names = names

    def type_of(self, base, suffix):
        """The struct_t a declaration names, or None."""
        for cand in (base + '_t', self.struct_names.get(base, '')):
            if cand in self.by_struct:
                return cand
        if suffix == '_t' and base + '_t' in self.by_struct:
            return base + '_t'
        return None

    def field_type(self, ctype):
        """(struct_t, is_pointer) of a field type, or (None, False)."""
        bare = ctype.replace('const ', '').strip()
        pointer = bare.endswith('*')
        bare = bare.rstrip('* ').replace('struct ', '').strip()
        if bare in self.by_struct:
            return bare, pointer
        if bare in self.struct_names and self.struct_names[bare] in self.by_struct:
            return self.struct_names[bare], pointer
        for suf, ptr in (('_ptr', True), ('_cptr', True)):
            if bare.endswith(suf) and bare[:-len(suf)] + '_t' in self.by_struct:
                return bare[:-len(suf)] + '_t', True
        return None, False

    def has(self, name):
        return name in self.declared


def environment(model, part):
    """{variable: (struct_t, is_pointer, is_const)} from the declarations in @p part."""
    env = {}
    for m in DECL.finditer(part):
        const, base, suffix, star, name = m.groups()
        tname = model.type_of(base, suffix or '')
        if not tname:
            continue
        pointer = '*' in star or suffix in ('_ptr', '_cptr')
        env[name] = (tname, pointer, bool(const) or suffix == '_cptr')
    return env


def context(text, start, end):
    """What surrounds the chain text[start:end]."""
    after = text[end:]
    before = text[:start]
    if re.match(r'\s*(?<![=!<>])=(?!=)', after):
        return 'write'
    m = re.match(r'\s*(\+|-|\*|/|\||&|\^|<<|>>|%)=', after)
    if m:
        return 'compound'
    if re.match(r'\s*\[', after):
        return 'index'
    if re.match(r'\s*(\+\+|--)', after) or re.search(r'(\+\+|--)\s*$', before):
        return 'step'
    if re.match(r'\s*\(', after):
        return 'call'
    if re.search(r'(lh_addr_of\(\s*|&\s*)$', before) and not re.search(r'&&\s*$', before):
        return 'address'
    return 'read'


def in_sizeof(text, start):
    depth = 0
    i = start - 1
    while i >= 0:
        c = text[i]
        if c == ')':
            depth += 1
        elif c == '(':
            if depth == 0:
                return text[:i].rstrip().endswith('sizeof')
            depth -= 1
        elif c in ';{}':
            return False
        i -= 1
    return False


def rewrite(model, part, left, path):
    env = environment(model, part)
    out = []
    pos = 0
    for m in CHAIN.finditer(part):
        if m.start() < pos:
            continue
        var = m.group(1)
        if var not in env:
            continue
        tname, pointer, const = env[var]
        segs = SEG.findall(m.group(2))
        if not segs or (segs[0][0] == '->') != pointer:
            continue
        ctx = context(part, m.start(), m.end())
        if in_sizeof(part, m.start()):
            left.append((path, part[:m.start()].count('\n'), m.group(0), 'sizeof'))
            continue
        mutable = ctx in ('write', 'compound', 'step', 'address', 'index') and not const
        if ctx == 'index':
            # The element is written by `=`, a compound assignment or a step, or taken by address.
            nxt = re.match(r'\s*\[[^\]]*\]\s*(?:(?<![=!<>])=(?!=)|(?:\+|-|\*|/|\||&|\^|<<|>>|%)=|\+\+|--)',
                           part[m.end():])
            before = part[:m.start()]
            stepped = (re.search(r'(\+\+|--|&|lh_addr_of\()\s*$', before)
                       and not re.search(r'&&\s*$', before))
            mutable = bool(nxt or stepped) and not const
        expr = var if pointer else f'lh_addr_of({var})'
        cur_t = tname
        done = 0
        result = None
        for i, (op, field) in enumerate(segs):
            fields = model.by_struct.get(cur_t, {})
            if field not in fields:
                break
            ctype, kind = fields[field]
            prefix = cur_t[:-1]
            last = i == len(segs) - 1
            if not last:
                if kind in ('view', 'owned'):
                    name = f'{prefix}get_{field}' + ('_mut' if mutable else '')
                    if not model.has(name):
                        break
                    expr = f'{name}({expr})'
                    cur_t, _ = model.field_type(ctype)
                    if cur_t is None or segs[i + 1][0] != '.':
                        break
                    done = i + 1
                    continue
                if kind == '' and ctype.rstrip().endswith('*'):
                    name = f'{prefix}get_{field}'
                    if not model.has(name):
                        break
                    expr = f'{name}({expr})'
                    cur_t, ptr = model.field_type(ctype)
                    if cur_t is None or segs[i + 1][0] != '->':
                        break
                    done = i + 1
                    continue
                break
            result = (prefix, field, kind, ctype)
            done = i + 1
        if result is None:
            continue
        prefix, field, kind, ctype = result
        consumed_all = done == len(segs)
        if not consumed_all:
            continue
        text_start, text_end = m.start(), m.end()
        get = {'flag': f'{prefix}is_{field}', 'fn': f'{prefix}get_{field}_fn'}.get(kind, f'{prefix}get_{field}')
        sett = f'{prefix}set_{field}' + ('_fn' if kind == 'fn' else '')
        replacement = None
        if ctx == 'write':
            stmt = re.match(r'\s*=\s*([^;]+);', part[text_end:])
            line_start = part.rfind('\n', 0, text_start) + 1
            if stmt and part[line_start:text_start].strip() == '' and model.has(sett) and \
                    kind not in ('array', 'owned'):
                value = ' '.join(stmt.group(1).split())
                replacement = f'{sett}({expr}, {value});'
                text_end += stmt.end()
        elif ctx in ('compound', 'step'):
            line_start = part.rfind('\n', 0, text_start) + 1
            head = part[line_start:text_start]
            cm = re.match(r'\s*(\+|-|\*|/|\||&|\^|<<|>>|%)=\s*([^;]+);', part[text_end:])
            sm = re.match(r'\s*(\+\+|--)\s*;', part[text_end:])
            pre = re.match(r'^\s*(\+\+|--)\s*$', head)
            if kind == '' and model.has(sett) and model.has(get):
                if cm and head.strip() == '':
                    replacement = f'{sett}({expr}, {get}({expr}) {cm.group(1)} ({" ".join(cm.group(2).split())}));'
                    text_end += cm.end()
                elif sm and head.strip() == '':
                    op = '+' if sm.group(1) == '++' else '-'
                    replacement = f'{sett}({expr}, {get}({expr}) {op} 1);'
                    text_end += sm.end()
                elif pre and re.match(r'\s*;', part[text_end:]):
                    op = '+' if pre.group(1) == '++' else '-'
                    text_start = line_start + len(head) - len(head.lstrip())
                    replacement = f'{sett}({expr}, {get}({expr}) {op} 1)'
        elif ctx == 'address':
            if kind in ('view', 'owned', 'array'):
                name = f'{prefix}get_{field}' + ('_mut' if mutable else '')
                if model.has(name):
                    before = part[:text_start]
                    am = re.search(r'(lh_addr_of\(\s*)$', before)
                    if am and part[text_end:].lstrip().startswith(')'):
                        text_start -= len(am.group(1))
                        text_end = text_end + part[text_end:].index(')') + 1
                        replacement = f'{name}({expr})'
                    elif re.search(r'&\s*$', before):
                        text_start = before.rstrip().rfind('&')
                        replacement = f'{name}({expr})'
        elif ctx == 'index':
            if kind == 'array':
                name = f'{prefix}get_{field}' + ('_mut' if mutable else '')
                if model.has(name):
                    replacement = f'{name}({expr})'
            elif kind == '' and ctype.rstrip().endswith('*') and model.has(get):
                replacement = f'{get}({expr})'
        elif ctx == 'call':
            if kind == 'fn' and model.has(get):
                replacement = f'{get}({expr})'
        else:
            if kind in ('view', 'owned') and model.has(f'{prefix}get_{field}'):
                replacement = f'lh_ptr_deref({prefix}get_{field}({expr}))'
            elif kind == 'array' and model.has(f'{prefix}get_{field}'):
                replacement = f'{prefix}get_{field}({expr})'
            elif kind in ('', 'flag', 'fn') and model.has(get):
                replacement = f'{get}({expr})'
        if replacement is None:
            left.append((path, part[:m.start()].count('\n'), m.group(0), ctx))
            continue
        out.append(part[pos:text_start])
        out.append(replacement)
        pos = text_end
    out.append(part[pos:])
    return ''.join(out)


def main():
    dry = '--dry' in sys.argv
    files = [a for a in sys.argv[1:] if not a.startswith('--')]
    root = os.path.abspath(os.path.join(HERE, '..', 'lib', 'lh'))
    model = Model([os.path.join(root, 'include'), os.path.join(root, 'src')])
    left = []
    for path in files:
        raw = open(path, 'rb').read().decode('utf-8')
        nl = '\r\n' if '\r\n' in raw else '\n'
        text = raw.replace('\r\n', '\n')
        parts = SPLIT.split(text)
        new = []
        for part in parts:
            if SPLIT.fullmatch(part) and not LEVEL1.search(part):
                # Run until nothing changes: an inner chain may hide an outer one.
                for _ in range(4):
                    again = rewrite(model, part, [], path)
                    if again == part:
                        break
                    part = again
                rewrite(model, part, left, path)
            new.append(part)
        text_new = ''.join(new)
        if not dry and text_new != text:
            open(path, 'wb').write(text_new.replace('\n', nl).encode('utf-8'))
    for path, line, chain, ctx in left:
        print(f'{os.path.relpath(path)}: {chain} ({ctx})')
    print(f'{len(left)} place(s) left for a hand fix.')
    return 0


if __name__ == '__main__':
    sys.exit(main())

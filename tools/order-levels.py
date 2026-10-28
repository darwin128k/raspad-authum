#!/usr/bin/env python3
"""Sort a module's definitions (.c) and declarations (.h) by level.

    python tools/order-levels.py <file.c> <file.h> <prefix>

Levels come from tools/check-levels.py: level 1 calls nothing of the module,
level N calls up to level N-1. Inside a level the current order is kept. A
definition keeps the comment above it; a declaration keeps its doc comment.
Only the function definitions of the .c and the declarations inside the
header's extern "C" block move. Line endings are kept.
"""

import importlib.util
import os
import re
import sys

here = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location('levels', os.path.join(here, 'check-levels.py'))
levels_mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(levels_mod)


def read(path):
    raw = open(path, 'rb').read().decode('utf-8')
    return raw.replace('\r\n', '\n'), ('\r\n' if '\r\n' in raw else '\n')


def write(path, text, newline):
    text = re.sub(r'\n{3,}', '\n\n', text)
    open(path, 'wb').write(text.replace('\n', newline).encode('utf-8'))


def split_items(text, pattern):
    """(head, [(name, chunk)], tail): chunks matched by @p pattern with what precedes them."""
    items = []
    pos = None
    head_end = None
    for m in pattern.finditer(text):
        if head_end is None:
            head_end = m.start()
            pos = m.start()
        items.append((m.group('name'), text[pos:m.end()]))
        pos = m.end()
    return text[:head_end], items, text[pos:]


def sort_items(items, level):
    keyed = [(level.get(name, 10 ** 6), i, name, chunk) for i, (name, chunk) in enumerate(items)]
    return [(name, chunk) for _, _, name, chunk in sorted(keyed)]


def main():
    c_path, h_path, prefix = sys.argv[1], sys.argv[2], sys.argv[3]
    graph, _ = levels_mod.functions(c_path, prefix)
    level, cycles = levels_mod.levels(graph)
    if cycles:
        print('cycles, not sorting:', cycles)
        return 1

    # .c: a definition with any comment and blank lines right above it.
    text, nl = read(c_path)
    definition = re.compile(
        r'(?:(?:/\*(?:(?!\*/).)*?\*/|//[^\n]*)\n|\n)*'
        r'[A-Za-z_][^\n;{}()]*\n(?P<name>' + re.escape(prefix) + r'\w+)\((?:[^;{]|\n)*?\)\n\{\n.*?\n\}\n',
        re.S)
    head, items, tail = split_items(text, definition)
    items = sort_items(items, level)
    write(c_path, head + ''.join('\n' + chunk.strip('\n') + '\n' for _, chunk in items) + tail, nl)

    # .h: the declarations inside the extern "C" block, each with its doc comment.
    text, nl = read(h_path)
    begin = text.index('LH_COMPILER_EXTERN_C_BEGIN\n') + len('LH_COMPILER_EXTERN_C_BEGIN\n')
    end = text.index('LH_COMPILER_EXTERN_C_END', begin)
    block = text[begin:end]
    declaration = re.compile(
        r'\s*(?:/\*\*(?:(?!\*/).)*?\*/\n)?[A-Za-z_][^\n;{}()]*\n(?P<name>' + re.escape(prefix) +
        r'\w+)\((?:[^;])*?\);\n', re.S)
    head, decls, tail = split_items(block, declaration)
    if tail.strip() or head.strip():
        print(f'{h_path}: something other than declarations in the block, not sorting it')
        return 1
    decls = sort_items(decls, level)
    block = '\n' + ''.join('\n' + chunk.strip('\n') + '\n' for _, chunk in decls) + '\n'
    write(h_path, text[:begin] + block + text[end:], nl)
    print(f'sorted {len(items)} definitions and {len(decls)} declarations of {prefix}')
    return 0


if __name__ == '__main__':
    sys.exit(main())

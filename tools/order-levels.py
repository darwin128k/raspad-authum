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


CONDITIONAL = re.compile(r'^\s*#\s*(?:if|ifdef|ifndef|elif|else|endif)\b', re.M)


def conditional(text):
    """True when @p text holds a preprocessor conditional: sorting across one moves code in or out of it."""
    return CONDITIONAL.search(text) is not None


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
    if items and conditional(''.join(chunk for _, chunk in items)):
        print(f'{c_path}: definitions sit inside #if blocks, not sorting (moving them would cross the blocks)')
        return 1
    if not items:
        print(f'{c_path}: no definition starts with {prefix}, not sorting')
        return 1
    items = sort_items(items, level)
    write(c_path, head + ''.join('\n' + chunk.strip('\n') + '\n' for _, chunk in items) + tail, nl)

    # .h: the declarations inside the extern "C" block, each with its doc comment.
    text, nl = read(h_path)
    declaration = re.compile(
        r'\s*(?:/\*\*(?:(?!\*/).)*?\*/\n)?[A-Za-z_][^\n;{}()]*\n(?P<name>' + re.escape(prefix) +
        r'\w+)\((?:[^;])*?\);\n', re.S)
    # The block that holds the module's declarations (others may hold only typedefs).
    blocks = [(m.end(), text.index('LH_COMPILER_EXTERN_C_END', m.end()))
              for m in re.finditer(r'LH_COMPILER_EXTERN_C_BEGIN\n', text)]
    with_decls = [(b, e) for b, e in blocks if declaration.search(text[b:e])]
    if len(with_decls) != 1:
        print(f'{h_path}: {len(with_decls)} blocks hold declarations, not sorting the header')
        return 1
    begin, end = with_decls[0]
    block = text[begin:end]
    head, decls, tail = split_items(block, declaration)
    if conditional(''.join(chunk for _, chunk in decls)):
        print(f'{h_path}: declarations sit inside #if blocks, not sorting the header')
        return 1
    if tail.strip():
        print(f'{h_path}: something other than declarations after them in the block, not sorting it')
        return 1
    # Function-type typedefs may open the block; they stay where they are.
    decls = sort_items(decls, level)
    block = ('\n' + head.strip('\n') + '\n' if head.strip() else '\n') + \
        ''.join('\n' + chunk.strip('\n') + '\n' for _, chunk in decls) + '\n'
    write(h_path, text[:begin] + block + text[end:], nl)
    print(f'sorted {len(items)} definitions and {len(decls)} declarations of {prefix}')
    return 0


if __name__ == '__main__':
    sys.exit(main())

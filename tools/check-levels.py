#!/usr/bin/env python3
"""Levels of a module's functions: who calls whom, from the field accessors up.

Level 1 calls no function of its own module (the getters and setters that
touch the fields). A function's level is one more than the highest level it
calls inside the module. Functions of the module below (e.g. view for
bounds) count as the floor, not as levels of this one.

    python tools/check-levels.py lib/lh/src/lh/memory/view.c lh_memory_view_
    python tools/check-levels.py <file.c> <prefix> --order   # is the file sorted by level?

Reports cycles (functions that reach each other), which have no level.
"""

import re
import sys


def functions(path, prefix):
    text = open(path, encoding='utf-8').read().replace('\r', '')
    found = {}
    order = []
    for m in re.finditer(r'^(' + re.escape(prefix) + r'\w+)\((?:[^;{]|\n)*?\)\n\{\n(.*?)\n\}\n',
                         text, re.S | re.M):
        name, body = m.group(1), m.group(2)
        calls = set(re.findall(r'\b(' + re.escape(prefix) + r'\w+)\s*\(', body)) - {name}
        found[name] = calls
        order.append(name)
    for name in found:
        found[name] &= set(found)
    return found, order


def levels(graph):
    level = {}
    visiting = set()
    cycles = []

    def visit(name, stack):
        if name in level:
            return level[name]
        if name in visiting:
            cycles.append(stack[stack.index(name):] + [name])
            return 0
        visiting.add(name)
        deepest = max((visit(callee, stack + [name]) for callee in graph[name]), default=0)
        visiting.discard(name)
        level[name] = deepest + 1
        return level[name]

    for name in graph:
        visit(name, [])
    return level, cycles


def main():
    path, prefix = sys.argv[1], sys.argv[2]
    graph, order = functions(path, prefix)
    level, cycles = levels(graph)
    for cycle in cycles:
        print('cycle: ' + ' -> '.join(cycle))
    if '--order' in sys.argv:
        out_of_order = [(a, b) for a, b in zip(order, order[1:]) if level[a] > level[b]]
        print(f'{path}: {len(out_of_order)} place(s) where a higher level comes before a lower one')
        return 1 if out_of_order or cycles else 0
    for depth in range(1, max(level.values(), default=0) + 1):
        names = [n for n in order if level[n] == depth]
        print(f'level {depth} ({len(names)}): ' + ' '.join(n[len(prefix):] for n in names))
    return 1 if cycles else 0


if __name__ == '__main__':
    sys.exit(main())

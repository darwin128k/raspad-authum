#!/usr/bin/env python3
"""Scan C sources for variables declared far from their use.

The rule: a variable is declared in the block where it is used. A run of
declarations at the top of a function body is the old C89 habit; C11 lets a
variable live next to the statement that needs it.

Heuristic (deliberately simple, no parser):
  * a declaration line starts with a type keyword or ends with ';' and has no
    '=' before the '(' of a function call,
  * a run = consecutive declaration lines at the start of a block,
  * a run of N or more declarations is reported.
"""

import argparse
import os
import re
import sys

TYPE = re.compile(
    r"^\s*(?:const\s+|static\s+|register\s+|volatile\s+|unsigned\s+|signed\s+"
    r"|struct\s+|union\s+|enum\s+|inline\s+)*"
    r"[A-Za-z_][A-Za-z0-9_]*(?:\s*\*)*\s+[A-Za-z_][A-Za-z0-9_, ]*\s*(?:\[[^\]]*\])?\s*;"
)
FUNC_START = re.compile(r"^\s*(?:[A-Za-z_][A-Za-z0-9_]*\s+)+\**[A-Za-z_][A-Za-z0-9_]*\s*\(")
SKIP = re.compile(r"^\s*(?://|/\*|\*|#|$)")


def is_declaration(text):
    if SKIP.match(text) or FUNC_START.match(text):
        return False
    if "=" in text and "(" not in text.split("=")[0]:
        return False
    return bool(TYPE.match(text))


TYPE_DEF = re.compile(r"^\s*(?:typedef\s+)?(?:struct|union|enum)\b")


def scan(path, limit):
    lines = open(path, encoding="utf-8", errors="replace").read().splitlines()
    depth = 0
    runs = []
    run = []
    type_block = 0  # depth of the block that holds a type definition
    pending_type = False
    for number, text in enumerate(lines, 1):
        stripped = text.strip()
        if depth == 1 and not type_block and is_declaration(text):
            run.append((number, stripped))
            continue
        if depth == 1 and run:
            runs.append(run)
            run = []
        if pending_type and "{" in text:
            type_block = depth + 1
            pending_type = False
        elif TYPE_DEF.match(text):
            pending_type = True
        depth += text.count("{") - text.count("}")
        if type_block and depth < type_block:
            type_block = 0
    if depth == 1 and run:
        runs.append(run)
    for group in runs:
        if len(group) >= limit:
            yield group


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("paths", nargs="+")
    parser.add_argument("--limit", type=int, default=2)
    args = parser.parse_args()

    files = []
    for path in args.paths:
        if os.path.isdir(path):
            for root, _, names in os.walk(path):
                for name in names:
                    if name.endswith((".c", ".h")):
                        files.append(os.path.join(root, name))
        else:
            files.append(path)

    total = 0
    for path in files:
        try:
            groups = list(scan(path, args.limit))
        except OSError as error:
            print(f"{path}: {error}", file=sys.stderr)
            continue
        for group in groups:
            total += 1
            first = group[0][0]
            print(f"{path}:{first}: {len(group)} declarations at the top of a block")
            for number, text in group:
                print(f"    {number}: {text}")
    if total:
        print(f"{total} place(s) with {args.limit} or more declarations at the top of a block.")
    return 1 if total else 0


if __name__ == "__main__":
    sys.exit(main())

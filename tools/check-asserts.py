#!/usr/bin/env python3
"""Report a contract check that runs twice for one pointer.

The rule: a pointer is checked once, at the boundary that receives it. A
function that checks its own argument and then hands the same pointer to
another function that checks it again pays for the check twice — and the inner
check can never fire, because the outer one already ended the program.

Heuristic (no parser, so it is a report and not a proof):
  * a function is a name followed by '(' at the start of a line,
  * a check is `lh_assert_runtime_ref(x)` (or `lh_assert_runtime_ifn` over a
    null test) inside that function,
  * a call is `g(x, ...)` where g is a function that checks its first
    parameter,
  * the report fires when the caller checks x and calls g(x, ...).
"""

import argparse
import os
import re
import sys

FUNC = re.compile(r"([A-Za-z_][A-Za-z0-9_]*)\s*\(")
ASSERT_REF = re.compile(r"lh_assert_runtime_ref\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)")
ASSERT_NULL = re.compile(r"lh_assert_runtime_ifn\s*\(\s*!\s*lh_null_eq\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)")
CALL = re.compile(r"\b([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\b")


def functions(path):
    """Yield (name, body_lines) for every function definition in the file."""
    lines = open(path, encoding="utf-8", errors="replace").read().splitlines()
    index = 0
    while index < len(lines):
        text = lines[index]
        match = FUNC.search(text)
        following = index + 1
        while following < len(lines) and not lines[following].strip():
            following += 1
        is_definition = (
            match
            and not text.rstrip().endswith(";")
            and following < len(lines)
            and lines[following].lstrip().startswith("{")
        )
        if not is_definition:
            index += 1
            continue
        name = match.group(1)
        body = []
        depth = 0
        started = False
        while index < len(lines):
            text = lines[index]
            body.append(text)
            depth += text.count("{") - text.count("}")
            if "{" in text:
                started = True
            index += 1
            if started and depth <= 0:
                break
        yield name, body


def checked_params(path):
    """The parameters each function checks, and the functions that check their first parameter."""
    first = {}
    for name, body in functions(path):
        params = set()
        for text in body:
            params.update(ASSERT_REF.findall(text))
            params.update(ASSERT_NULL.findall(text))
        first[name] = params
    return first


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("paths", nargs="+")
    args = parser.parse_args()

    files = []
    for path in args.paths:
        if os.path.isdir(path):
            for root, _, names in os.walk(path):
                for name in names:
                    if name.endswith(".c"):
                        files.append(os.path.join(root, name))
        else:
            files.append(path)

    total = 0
    for path in files:
        try:
            checks = checked_params(path)
        except OSError as error:
            print(f"{path}: {error}", file=sys.stderr)
            continue
        for name, body in functions(path):
            own = set()
            for text in body:
                own.update(ASSERT_REF.findall(text))
                own.update(ASSERT_NULL.findall(text))
            if not own:
                continue
            for number, text in enumerate(body, 1):
                for callee, argument in CALL.findall(text):
                    if callee == name or argument not in own:
                        continue
                    if argument in checks.get(callee, ()):
                        total += 1
                        print(f"{path}: {name} checks {argument} and calls {callee}({argument}), "
                              f"which checks it again")
    if total:
        print(f"{total} double check(s).")
    return 1 if total else 0


if __name__ == "__main__":
    sys.exit(main())

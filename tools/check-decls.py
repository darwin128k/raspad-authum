#!/usr/bin/env python3
"""Scan C sources for variables declared far from their use.

Two rules:

  * a variable is declared in the block where it is used. A run of declarations
    at the top of a function body is the old C89 habit; C11 lets a variable live
    next to the statement that needs it.
  * a loop counter that belongs to the loop alone lives in the `for` header.
    A separate declaration above the loop is right only when the variable
    outlives the loop and is read after it.

Heuristic (deliberately simple, no parser):
  * a declaration line starts with a type keyword or ends with ';' and has no
    '=' before the '(' of a function call,
  * a run = consecutive declaration lines at the start of a block,
  * a run of N or more declarations is reported,
  * a plain declaration right above a `for` whose header names that variable,
    with no later use of it in the same block, is reported as a loop counter.
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
TYPE_DEF = re.compile(r"^\s*(?:typedef\s+)?(?:struct|union|enum)\b")

PLAIN_DECL = re.compile(
    r"^\s*(?:const\s+|static\s+|register\s+|volatile\s+|unsigned\s+|signed\s+)*"
    r"[A-Za-z_][A-Za-z0-9_]*(?:\s*\*)*\s+([A-Za-z_][A-Za-z0-9_]*)\s*(?:\[[^\]]*\])?"
    r"\s*(?:=[^;]*)?;$"
)
FOR = re.compile(r"^\s*for\s*\(")
DECL_IN_FOR = re.compile(
    r"^\s*(?:const\s+|static\s+|register\s+|volatile\s+|unsigned\s+|signed\s+)*"
    r"[A-Za-z_][A-Za-z0-9_]*\s+\*?\s*[A-Za-z_][A-Za-z0-9_]*\s*="
)


def is_declaration(text):
    if SKIP.match(text) or FUNC_START.match(text):
        return False
    if "=" in text and "(" not in text.split("=")[0]:
        return False
    return bool(TYPE.match(text))


def strip_noise(text):
    """Blank out comments and literal contents, keeping the line structure."""
    out = []
    index = 0
    size = len(text)
    while index < size:
        char = text[index]
        if char == "/" and index + 1 < size and text[index + 1] == "/":
            while index < size and text[index] != "\n":
                out.append(" ")
                index += 1
            continue
        if char == "/" and index + 1 < size and text[index + 1] == "*":
            out.append("  ")
            index += 2
            while index < size and not (
                text[index] == "*" and index + 1 < size and text[index + 1] == "/"
            ):
                out.append("\n" if text[index] == "\n" else " ")
                index += 1
            out.append("  ")
            index += 2
            continue
        if char in "\"'":
            quote = char
            out.append(" ")
            index += 1
            while index < size and text[index] != quote:
                if text[index] == "\\" and index + 1 < size:
                    out.append("  ")
                    index += 2
                    continue
                out.append("\n" if text[index] == "\n" else " ")
                index += 1
            if index < size:
                out.append(" ")
                index += 1
            continue
        out.append(char)
        index += 1
    return "".join(out)


def matching(text, start, open_char, close_char):
    """Offset of the close char that matches the open char at start."""
    depth = 0
    for index in range(start, len(text)):
        if text[index] == open_char:
            depth += 1
        elif text[index] == close_char:
            depth -= 1
            if depth == 0:
                return index
    return -1


def statement_end(clean, offset):
    """Offset just past the statement that starts at offset."""
    size = len(clean)
    while offset < size and clean[offset] in " \t\r\n":
        offset += 1
    if offset < size and clean[offset] == "{":
        close = matching(clean, offset, "{", "}")
        return close + 1 if close >= 0 else size
    while offset < size:
        if clean[offset] == ";":
            return offset + 1
        if clean[offset] == "{":
            close = matching(clean, offset, "{", "}")
            return close + 1 if close >= 0 else size
        offset += 1
    return size


def block_end(clean, offset):
    """Offset of the close brace of the block that holds offset."""
    depth = 0
    for index in range(offset, len(clean)):
        char = clean[index]
        if char == "{":
            depth += 1
        elif char == "}":
            if depth == 0:
                return index
            depth -= 1
    return len(clean)


def scan_piles(path, limit):
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


def scan_loop_counters(path):
    """Report a counter declared above the `for` that uses it and only it."""
    text = open(path, encoding="utf-8", errors="replace").read()
    clean = strip_noise(text)
    lines = clean.splitlines()
    raw = text.splitlines()
    offsets = []
    position = 0
    for line in lines:
        offsets.append(position)
        position += len(line) + 1
    findings = []
    for index, line in enumerate(lines):
        if not FOR.match(line):
            continue
        previous = index - 1
        while previous >= 0 and not lines[previous].strip():
            previous -= 1
        if previous < 0:
            continue
        match = PLAIN_DECL.match(lines[previous])
        if not match:
            continue
        name = match.group(1)
        paren = offsets[index] + line.index("(")
        paren_end = matching(clean, paren, "(", ")")
        if paren_end < 0:
            continue
        header = clean[paren + 1 : paren_end]
        clauses = header.split(";")
        init = clauses[0] if clauses else ""
        if DECL_IN_FOR.match(init):
            continue
        if not re.search(r"\b%s\b" % re.escape(name), header):
            continue
        end = statement_end(clean, paren_end)
        rest = clean[end : block_end(clean, end)]
        if re.search(r"\b%s\b" % re.escape(name), rest):
            continue
        findings.append((previous + 1, index + 1, name, raw[previous].strip()))
    return findings


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
            groups = list(scan_piles(path, args.limit))
            counters = scan_loop_counters(path)
        except OSError as error:
            print(f"{path}: {error}", file=sys.stderr)
            continue
        for group in groups:
            total += 1
            first = group[0][0]
            print(f"{path}:{first}: {len(group)} declarations at the top of a block")
            for number, text in group:
                print(f"    {number}: {text}")
        for declaration, loop, name, text in counters:
            total += 1
            print(
                f"{path}:{declaration}: loop counter '{name}' is declared above the loop"
                f" ({path}:{loop})"
            )
            print(f"    {declaration}: {text}")
    if total:
        print(f"{total} place(s) to fix.")
    return 1 if total else 0


if __name__ == "__main__":
    sys.exit(main())

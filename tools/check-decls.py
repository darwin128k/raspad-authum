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
  * a run of N or more declarations is reported, unless the statement right
    after it uses every variable of the run: storage handed to one call (a
    buffer and the object that keeps it) is declared where it is used,
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


DECL_NAME = re.compile(r"([A-Za-z_][A-Za-z0-9_]*)\s*(?:\[[^\]]*\])?\s*;\s*$")


def next_statement(lines, index):
    """Text of the statement that starts on the first non-blank line at index.

    A compound statement counts whole: its braced body, and the `else` branches
    that follow it. Storage a loop or an `if` needs is declared right before it.
    """
    text = ""
    paren = 0
    brace = 0
    opened = False
    compound = False
    position = index
    while position < len(lines):
        clean = strip_noise(lines[position])
        position += 1
        if not text and not clean.strip():
            continue
        if not text:
            compound = bool(re.match(r"\s*(?:for|while|if|switch|do)\b", clean))
        if opened and brace == 0:
            if re.match(r"\s*else\b", clean):
                opened = False
            else:
                break
        text += " " + clean
        paren += clean.count("(") - clean.count(")")
        brace += clean.count("{") - clean.count("}")
        if "{" in clean:
            opened = True
        # A plain statement ends at its `;`; a loop or an `if` header holds `;`
        # of its own, so it ends with its body.
        if not opened and paren <= 0 and brace == 0 and clean.rstrip().endswith(";"):
            if not compound or not re.match(r"\s*(?:for|while|if|switch|do)\b", clean):
                break
    return text


def used_together(lines, group):
    """True when the statement after the run names every variable of it."""
    after = group[-1][0]  # 1-based number of the last line == index of the next one
    statement = next_statement(lines, after)
    for _, text in group:
        match = DECL_NAME.search(strip_noise(text))
        if not match or not re.search(r"\b%s\b" % re.escape(match.group(1)), statement):
            return False
    return True


def scan_piles(path, limit):
    lines = open(path, encoding="utf-8", errors="replace").read().replace("\r\n", "\n").split("\n")
    # A body's own statements: one code block deep, not inside a type, with
    # `namespace` / `extern "C"` transparent and braces in comments not counted.
    _, shape = block_shape(lines)
    runs = []
    run = []
    for number, text in enumerate(lines, 1):
        depth, in_type = shape[number - 1]
        in_body = depth == 1 and not in_type
        if in_body and is_declaration(text):
            run.append((number, text.strip()))
            continue
        if run and (in_body or depth == 0):
            runs.append(run)
            run = []
    if run:
        runs.append(run)
    for group in runs:
        if len(group) >= limit and not used_together(lines, group):
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
        # A counter is what the init clause of a three-clause `for` assigns; the
        # array a range-for walks (`for (T v : values)`) is not one.
        if len(clauses) != 3 or not re.match(r"\s*%s\s*=[^=]" % re.escape(name), init):
            continue
        end = statement_end(clean, paren_end)
        rest = clean[end : block_end(clean, end)]
        if re.search(r"\b%s\b" % re.escape(name), rest):
            continue
        findings.append((previous + 1, index + 1, name, raw[previous].strip()))
    return findings


# `p = &x` / `p = lh_addr_of(x)`; a member on the left (`c.field = &x`) stores
# into an object, not into a pointer variable, and is not counted.
ESCAPE = re.compile(
    r"(?<![.>\w])([A-Za-z_][A-Za-z0-9_]*)\s*=\s*"
    r"(?:lh_addr_of\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)|&\s*([A-Za-z_][A-Za-z0-9_]*))"
)
ESCAPE_OUT = re.compile(r"lh_ptr_deref\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*=\s*lh_addr_of")


def block_shape(lines):
    """Per line: the depth of code blocks at its start and whether it sits in a type.

    Braces in comments and literals do not count; `namespace` and `extern "C"`
    blocks are transparent; a struct / class / union / enum body holds members,
    not locals.
    """
    clean_lines = strip_noise("\n".join(lines)).split("\n")
    stack = []
    shape = []
    for index, clean in enumerate(clean_lines):
        code = sum(1 for kind in stack if kind == "code")
        shape.append((code, bool(stack) and stack[-1] == "type"))
        for char in clean:
            if char == "{":
                head = " ".join(clean_lines[max(0, index - 3) : index] + [clean[: clean.index("{")]])
                cut = max(head.rfind(";"), head.rfind("}"), head.rfind("{"))
                opener = head[cut + 1 :]
                if re.search(r"\b(?:namespace|extern)\b", opener):
                    stack.append("ns")
                elif re.search(r"\b(?:struct|class|union|enum)\b", opener) and "(" not in opener:
                    stack.append("type")
                else:
                    stack.append("code")
            elif char == "}" and stack:
                stack.pop()
    return clean_lines, shape


def params_of(lines, index):
    """Names of the parameters of the function that owns the given line."""
    text = ""
    for back in range(index, max(-1, index - 60), -1):
        line = lines[back]
        if not text and line.strip() == "{":
            continue
        text = line + " " + text
        if ")" in line:
            break
    if "(" not in text or ")" not in text:
        return []
    head = text.index("(")
    tail = text.index(")", head)
    names = []
    for part in text[head + 1 : tail].split(","):
        words = re.findall(r"[A-Za-z_][A-Za-z0-9_]*", part)
        if words:
            names.append(words[-1])
    return names


def scan_escaping_addresses(path):
    """Report a local whose address is taken and outlives its block."""
    lines = open(path, encoding="utf-8", errors="replace").read().replace("\r\n", "\n").split("\n")
    clean_lines, shape = block_shape(lines)
    findings = []
    for index, line in enumerate(lines):
        inner, in_type = shape[index]
        if inner < 2 or in_type:
            continue
        match = PLAIN_DECL.match(line)
        if not match or "typedef" in line:
            continue
        name = match.group(1)
        params = params_of(lines, index)
        # The block ends where the code depth falls below the declaration's;
        # the function ends where it reaches zero.
        block_end = next(
            (i for i in range(index + 1, len(lines)) if shape[i][0] < inner), len(lines)
        )
        function_end = next(
            (i for i in range(block_end, len(lines)) if shape[i][0] == 0), len(lines)
        )
        for follow in range(index + 1, block_end):
            text = clean_lines[follow]
            targets = [
                hit.group(1)
                for hit in ESCAPE.finditer(text)
                if (hit.group(2) or hit.group(3)) == name
            ]
            targets += [hit.group(1) for hit in ESCAPE_OUT.finditer(text) if name in text]
            reported = False
            for pointer in targets:
                after = "\n".join(clean_lines[block_end:function_end])
                if pointer in params or re.search(r"\b%s\b" % re.escape(pointer), after):
                    findings.append((index + 1, name, follow + 1, lines[follow].strip()))
                    reported = True
                    break
            if reported:
                break
    return findings

MOVABLE = re.compile(
    r"^(\s*)((?:const\s+|volatile\s+|unsigned\s+|signed\s+|struct\s+|union\s+|enum\s+)*"
    r"[A-Za-z_][A-Za-z0-9_:<>]*(?:\s*\*)*)\s*\b([A-Za-z_][A-Za-z0-9_]*)\s*(\[[^\]]*\])?\s*;\s*$"
)
PLAIN_KIND = re.compile(
    r"^(?:const\s+|volatile\s+|unsigned\s+|signed\s+|struct\s+)*"
    r"(?:[A-Za-z_][A-Za-z0-9_]*_t|char|int|long|short|bool|float|double|va_list)(?:\s*\*)*$"
)
LABEL = re.compile(r"^\s*[A-Za-z_][A-Za-z0-9_]*\s*:\s*$")


def significant(clean):
    return bool(clean.strip())


def body_ranges(lines, clean_lines):
    """(open, close) line indexes of every brace block that is a function body.

    `namespace` and `extern "C"` blocks are transparent: their insides count as
    top level. Type definitions (struct, class, union, enum) are skipped whole.
    """
    depth = 0
    ranges = []
    start = None
    skipping = False
    for index, clean in enumerate(clean_lines):
        before = depth
        depth += clean.count("{") - clean.count("}")
        if before == 0 and depth >= 1 and start is None and not skipping:
            head = " ".join(clean_lines[max(0, index - 3) : index + 1])
            cut = max(head.rfind(";"), head.rfind("}"))
            opener = head[cut + 1 :] if cut >= 0 else head
            if re.search(r"\b(?:namespace|extern)\b", opener):
                depth = 0
                continue
            if re.search(r"\b(?:struct|class|union|enum)\b", opener):
                skipping = True
                if depth == 0:
                    skipping = False
                continue
            start = index
        if depth <= 0:
            depth = 0
            if skipping:
                skipping = False
            elif start is not None:
                ranges.append((start, index))
                start = None
    return ranges


def statement_starts(clean_lines, open_index, close_index):
    """Indexes of the lines that start a statement directly in the body."""
    starts = []
    depth = 1
    paren = 0
    boundary = True
    for index in range(open_index + 1, close_index):
        clean = clean_lines[index]
        # A preprocessor line is not a statement and leaves the boundary as it
        # was: a declaration put before `#endif` would land inside the branch.
        if clean.lstrip().startswith("#"):
            continue
        if depth == 1 and paren == 0 and boundary and significant(clean):
            # `else` and `catch` continue the statement above them.
            if not re.match(r"\s*(?:else|catch)\b", clean):
                starts.append(index)
            boundary = False
        depth += clean.count("{") - clean.count("}")
        paren += clean.count("(") - clean.count(")")
        tail = clean.rstrip()
        if depth == 1 and paren == 0 and tail and (tail[-1] in ";{}" or LABEL.match(clean)):
            boundary = True
    return starts


def loop_end(clean_lines, index, close_index):
    """Index of the last line of the `for` statement that starts at index."""
    depth = 0
    paren = 0
    opened = False
    for line in range(index, close_index):
        clean = clean_lines[line]
        paren += clean.count("(") - clean.count(")")
        if "{" in clean:
            opened = True
        depth += clean.count("{") - clean.count("}")
        if opened and depth == 0:
            return line
        if not opened and paren == 0 and clean.rstrip().endswith(";") and line > index:
            return line
    return close_index


def fix_file(path, objects=False, moved_kinds=None):
    """Move each plain declaration of a body down to the statement that first uses it."""
    raw = open(path, "rb").read().decode("utf-8")
    newline = "\r\n" if "\r\n" in raw else "\n"
    lines = raw.replace("\r\n", "\n").split("\n")
    clean_lines = strip_noise("\n".join(lines)).split("\n")
    moves = {}  # target index -> [declaration lines]
    removed = set()
    rewrite = {}  # target index -> new text
    for open_index, close_index in body_ranges(lines, clean_lines):
        starts = statement_starts(clean_lines, open_index, close_index)
        for index in starts:
            text = lines[index]
            match = MOVABLE.match(text)
            if not match or "static" in text or "typedef" in text or "return" in text:
                continue
            if not is_declaration(text):
                continue
            indent, kind, name, array = match.groups()
            if kind.strip() in ("goto", "case", "else", "delete", "throw"):
                continue
            plain = PLAIN_KIND.match(kind.strip()) or not path.endswith(".cpp")
            if not plain and not objects:
                continue  # a C++ object may do work in its constructor: by hand
            pattern = re.compile(r"\b%s\b" % re.escape(name))
            first = None
            for follow in range(index + 1, close_index):
                if pattern.search(clean_lines[follow]):
                    first = follow
                    break
            if first is None:
                # Never used: a plain declaration with no initializer does nothing.
                if plain:
                    removed.add(index)
                continue
            # A counter shared by several loops: each `for (name = ...)` sets it, and
            # nothing outside those loops reads it, so each loop gets its own.
            uses = [i for i in range(index + 1, close_index) if pattern.search(clean_lines[i])]
            loops = [
                i
                for i in uses
                if re.match(r"\s*for\s*\(\s*%s\s*=[^=]" % re.escape(name), clean_lines[i])
            ]
            if not array and loops and not any(i in rewrite for i in loops):
                spans = [(i, loop_end(clean_lines, i, close_index)) for i in loops]
                if all(any(a <= u <= b for a, b in spans) for u in uses) and not any(
                    lines[i].lstrip().startswith("#") for i in range(index + 1, spans[-1][1])
                ):
                    for i in loops:
                        rewrite[i] = re.sub(
                            r"for\s*\(\s*%s\s*=" % re.escape(name),
                            "for (%s %s =" % (kind.strip(), name),
                            lines[i],
                            count=1,
                        )
                    removed.add(index)
                    continue
            target = max(i for i in starts if i <= first)
            if target <= index:
                continue
            if any(lines[i].lstrip().startswith("#") for i in range(index + 1, target)):
                continue  # never move a declaration across a preprocessor branch
            statement = lines[target]
            stripped = statement.strip()
            kind = kind.strip()
            if not array and target not in rewrite and target not in moves:
                if re.match(r"%s\s*=[^=]" % re.escape(name), stripped):
                    rewrite[target] = "%s%s %s" % (indent, kind, stripped)
                    removed.add(index)
                    continue
                loop = re.match(r"for\s*\(\s*%s\s*=[^=]" % re.escape(name), stripped)
                if loop:
                    end = loop_end(clean_lines, target, close_index)
                    rest = "\n".join(clean_lines[end + 1 : close_index])
                    if not pattern.search(rest) and not pattern.search(clean_lines[end]):
                        rewrite[target] = re.sub(
                            r"for\s*\(\s*%s\s*=" % re.escape(name),
                            "for (%s %s =" % (kind, name),
                            statement,
                            count=1,
                        )
                        removed.add(index)
                        continue
            between = [i for i in starts if index < i < target]
            if all(MOVABLE.match(lines[i]) and i not in rewrite for i in between):
                continue  # only declarations in between: it already sits there
            moves.setdefault(target, []).append(text)
            removed.add(index)
            if not plain and moved_kinds is not None:
                moved_kinds.add(kind.strip())
    if not removed:
        return 0
    if any(target in removed for target in list(moves) + list(rewrite)):
        print(f"{path}: a declaration would move onto another moved one; left as is")
        return 0
    # A removed pile leaves its blank line behind: that blank line goes when it
    # would now follow `{` or another blank line. Blank lines elsewhere stay.
    out = []
    after_removed = False
    for index, text in enumerate(lines):
        if index in moves:
            out.extend(moves[index])
        if index in removed:
            after_removed = True
            continue
        if (
            after_removed
            and not text.strip()
            and out
            and (not out[-1].strip() or out[-1].rstrip().endswith("{"))
        ):
            after_removed = False
            continue
        after_removed = False
        out.append(rewrite.get(index, text))
    open(path, "wb").write(newline.join(out).encode("utf-8"))
    return len(removed)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("paths", nargs="+")
    parser.add_argument("--limit", type=int, default=2)
    parser.add_argument(
        "--fix",
        action="store_true",
        help="move plain declarations down to the statement that first uses them",
    )
    parser.add_argument(
        "--objects",
        action="store_true",
        help="with --fix, move C++ objects too, and list their types for review: a"
        " constructor that does work elsewhere (a scope guard) must keep its place",
    )
    args = parser.parse_args()

    files = []
    for path in args.paths:
        if os.path.isdir(path):
            for root, _, names in os.walk(path):
                for name in names:
                    if name.endswith((".c", ".h", ".cpp")):
                        files.append(os.path.join(root, name))
        else:
            files.append(path)

    if args.fix:
        moved = 0
        kinds = set()
        for path in files:
            moved += fix_file(path, args.objects, kinds)
        print(f"{moved} declaration(s) moved.")
        if kinds:
            print("C++ types moved (check their constructors): " + ", ".join(sorted(kinds)))
        return 0

    total = 0
    for path in files:
        try:
            groups = list(scan_piles(path, args.limit))
            counters = scan_loop_counters(path)
            escapes = scan_escaping_addresses(path)
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
        for declaration, name, use, text in escapes:
            total += 1
            print(
                f"{path}:{declaration}: the address of '{name}' leaves its block"
                f" ({path}:{use})"
            )
            print(f"    {use}: {text}")
    if total:
        print(f"{total} place(s) to fix.")
    return 1 if total else 0


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3
"""Check Authum foundation boundaries without third-party parser dependencies."""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
TOKENS = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')


def mask(text, strings):
    def replace(match):
        token = match.group()
        if token.startswith(("//", "/*")) or strings:
            return "".join("\n" if char == "\n" else " " for char in token)
        return token
    return TOKENS.sub(replace, text)


def main():
    errors = []
    paths = [path for folder in ("src/authum", "include/authum", "examples")
             for path in (ROOT / folder).rglob("*") if path.suffix in (".c", ".cpp", ".h")]
    rules = [
        (r'(?<![\w>.])(?:std::)?(?:malloc|calloc|realloc|free)\s*\(',
         "use authum_memory_* or the matching sandbox API"),
        (r'\b(?:std::)?(?:memset|memcpy|memmove|memcmp)\s*\(', "use LH memory kernels"),
        (r'\bNULL\b', "use authum_null"),
        (r'\b(?:static_cast|reinterpret_cast|const_cast)\s*<', "use LH cast helpers"),
        (r'\bfor\s*\(\s*(?:unsigned|int|size_t)\b', "use an Authum loop-index type"),
        (r'\b65535\b', "use the named port/type limit"),
    ]
    for path in paths:
        raw = path.read_bytes()
        relative = path.relative_to(ROOT).as_posix()
        text = raw.decode("utf-8")
        uncommented, code = mask(text, False), mask(text, True)
        active = list(rules)
        if relative.startswith("src/authum/"):
            active.append((r'\b(?:const\s+)?void\s*\*', "use authum_ptr/authum_cptr"))
        if relative.startswith("src/authum/") and relative != "src/authum/api/memory.c":
            active.append((r'\blh_runtime_allocator_(?:alloc|realloc|free)\s*\(',
                           "route explicit Authum heap blocks through its memory facade"))
        if b"\r\r\n" in raw:
            errors.append(f"{relative}: duplicated CR line endings")
        for pattern, reason in active:
            for match in re.finditer(pattern, code):
                line = code.count("\n", 0, match.start()) + 1
                errors.append(f"{relative}:{line}: {reason}")
        for match in re.finditer(r'^\s*#\s*include\s+"', uncommented, re.MULTILINE):
            line = uncommented.count("\n", 0, match.start()) + 1
            errors.append(f"{relative}:{line}: use an angle-bracket include with its namespace path")
        if relative.startswith("include/authum/") and re.search(r'^\s*#\s*include\s*<lh/', uncommented, re.MULTILINE):
            errors.append(f"{relative}: public Authum headers must not expose LH headers")
    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1
    print(f"Foundation checks passed for {len(paths)} Authum files.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

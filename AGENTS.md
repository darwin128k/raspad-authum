# Concept — read first

These are the owner's standing requirements. They are repeated in every
session; do not make the owner repeat them again.

- **LH is the box of bricks; Authum is the mechanism built from them.**
  Authum is a transparent facade over LH: gamedll proxy, module loader,
  module lifecycle and host-owned state (sandbox, events, commands, tasks,
  shared HTTP service, log channels, placeholders). Nothing generic is
  implemented in Authum. If a brick is missing, add it to LH first, then
  wrap it as a thin `authum_*` facade with no logic of its own.
- **The LH rules apply to Authum code too:** one file — one entity, decompose
  and delegate, no parallel implementations or detours, `try_` + checked
  pairs, names that say what they return, a test per brick, full briefs.
  They are in [lib/lh/AGENTS.md](lib/lh/AGENTS.md) — read it.
- **Open work** lives in [docs/cleanup.md](docs/cleanup.md). Check it before
  starting; add debt you find there instead of fixing it silently.
- Do not commit unless the owner asks. Commits carry no AI attribution.

# Authum implementation conventions

- Use Authum types and `authum_null` in Authum code; keep native types at actual ABI boundaries.
- Allocate explicit-lifetime heap blocks through `<authum/memory.h>`, backed by LH.
  Keep sandbox ownership on the existing LH ownership tree; never mix the two kinds of blocks.
- Use LH memory kernels, math operations, cast and pointer helpers, character predicates,
  ASCII names and numeric/domain limits instead of open-coding equivalent primitives.
- Use `lh_fs_path_t` for private LH filesystem operations above `lh/os/system`;
  keep `authum_path_t` on the public Authum ABI. Raw native path text belongs at the kernel boundary. Preserve unresolved placeholder text as an Authum policy.
- Include project headers with angle brackets and namespace paths. Keep module example
  contracts in that module's include namespace, not in the Authum host API.
- Preserve line endings when scripting edits; inspect the diff for accidental blank lines.
- Run `python tools/check-conventions.py` after foundation or include changes.

# Contributions to LH

Before changing `lib/lh`, read the nearest existing headers and implementations
for that subsystem, together with `lib/lh/.clang-format`.

- Follow LH types, pointer/cast helpers, field macros and callback `fn`/`cb` layout.
- Use LH memory, allocator, error and runtime facilities in portable code.
  Native APIs and native types belong in the matching OS backend.
- Document public functions, ownership, failure results and lifecycle constraints
  in the same form as neighboring LH headers.
- Extend an existing LH abstraction before introducing a parallel one.
- Run the formatter using LH's own configuration, inspect the diff manually,
  and test the affected primitives and their Authum integration.
  Formatting alone does not establish conformity with LH conventions.

- LH C/C++ unit tests use GoogleTest in `test/src/lh/` and the common runner. Initialize the
  pinned GoogleTest submodule when needed; do not add standalone test runners,
  manual `CHECK` macros, abort-based assertions or parallel assertion frameworks.
  C-only compile fixtures may remain when exercised by the GoogleTest targets.

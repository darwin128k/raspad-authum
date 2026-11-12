# Concept — read first

These are the owner's standing requirements. They are repeated in every
session; do not make the owner repeat them again.

- **LH is the box of bricks; Authum is a thin game facade over them.**
  Every mechanism that is not about the game — the module stack
  (`lh/os/shared` → `lh/os/module` + `lh/os/loader` → `lh/os/module/event`),
  console commands, tasks, threads, logging, configuration, paths, memory
  ownership — lives in LH, so other projects (a 3D editor with DLL modules, ...)
  take the same bricks. Each level is usable without the ones above it.
  Authum keeps only the game: the gamedll proxy, the engine, players, game
  events and messages, and the module ABI. If a brick is missing, add it to LH
  first, then wrap it as a thin `authum_*` facade with no logic of its own.
- **Authum has a facade of its own; LH never shows through it.** Authum's
  sources use LH freely, but no public `include/authum/` header includes an LH
  header or names an LH type: modules see only `authum_*` types and functions,
  and are built without LH.
- **The module ABI is 4.0 (Authum 4.1.0).** Within a major nothing that exists
  changes or moves. A new major is the owner's decision, and it gathers every
  breaking change into one release, so modules are rewritten once (4.0: the
  region, the opaque array, no `FILE *`). New API is
  only appended — to the end of a host table, or as a new table or a new
  structure — and bumps the minor. A structure a module fills in never grows: a
  new field is a new structure. Host tables carry no size; a module needs a host
  of the same major and the same or a later minor
  (`authum_version_is_abi_compatible`). Host tables are built with designated
  initializers.
- **Names say what they return** — in Authum's public API as in LH: a value is
  `get_x` / `is_x` / `has_x`; a result written into the caller's buffer or
  structure is `copy_x`; a scalar written through a pointer with a success flag
  is `try_get_x`; an action is a verb (`kick`, `print`, `precache_model`). A
  reader must know what comes back without opening the header.
- **Same style as LH**: the same layout, briefs and `.clang-format`; format the
  changed lines with it (`git-clang-format`, see `docs/cleanup.md`).
- **No raw fields, anywhere.** A struct's fields are touched only by its own
  level-1 getters / setters; everything else — its own `.c` file included,
  Authum and LH alike — goes through them, and functions are layered (level 1
  touches fields, level 2 uses level 1, ...). Full rule: `lib/lh/AGENTS.md`
  section 0.0.
- **First rule: use the bricks that exist.** The owner: "Why did we create all
  these wrappers at all, if nobody uses them — not even LH itself?" Code that
  re-implements an existing LH brick (a character literal instead of
  `lh/core/char/map.h`, a `switch` instead of `lh_str_ptr_contains_char`, a digit
  loop instead of `lh/core/str/format`) is a defect. Search first; fix the brick if
  it does not fit; never write a local copy.
- **Operators are not bricks; meanings are** (owner, 2026-11-11). A plain
  comparison or arithmetic is written with its operator: `a == b`, `a + b * c`,
  `++i`. A wrapper that is only the operator again (`lh_math_eq`, `lh_math_add`,
  `lh_math_add_one`) is allowed but not required, and existing ones are not
  rewritten either way. What has a brick of its own is always written with it:
  min / max / abs / clamp (`lh/math`), overflow (`lh_interval_*_is_*_overflow`),
  pointer arithmetic and comparison (`lh_ptr_*`), a null check of a pointer
  (`lh_ptr_is_null` / `lh_null_eq`, `authum_null_eq` — never `!p`, which does not
  say whether `p` is a pointer or a flag), characters (`lh_char_map_*`,
  `lh_char_is_*`), and casts (`lh_cast_*`).
- **Sources mirror headers, exactly.** `include/lh/<path>.h` is implemented by
  `src/lh/<path>.c`, `include/authum/<path>.h` by `src/authum/api/<path>.c`: the same
  folders, the same names. Nesting is a folder, never a name glued with `_`
  (`command/def.c`, not `command_def.c`). Check with `python tools/check-mirror.py`
  (0 required).
- **From smaller to larger, no extra includes.** Each brick builds only on
  smaller ones; a file never includes a header that one of its includes
  already brings in. Check with `python tools/check-includes.py` (0 required).
- **`extern "C"` wraps only what has linkage** (functions, `extern` objects,
  function-type typedefs); structs, enums, plain typedefs and macros go before
  it. Check with `python tools/check-extern-c.py` (0 required). `gamedll/` is
  C++ and keeps C++ linkage.
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
  Keep region ownership on the existing LH ownership tree; never mix the two kinds of blocks.
- Use LH memory kernels, math operations, cast and pointer helpers, character predicates,
  ASCII names and numeric/domain limits instead of open-coding equivalent primitives.
- **Everything runs on the LH core — all of it.** Every brick Authum uses is an lh
  brick, and there is no second road. If something in Authum falls behind on speed,
  the problem is in lh: fix lh (measure before and after with `bench/`, and write
  the reason and the numbers into the root's documentation and `CHANGELOG.md`).
  **Never write a local workaround to be faster** — a parallel implementation is a
  defect, not a solution, and nobody can later tell why it exists.
- A field name does not repeat its context. The module identity carries `tag` (how
  the module is marked in the logs), not `logtag`: the struct already says what it
  is, and the neighbours `name`, `author`, `url` repeat nothing either.
- **Three words, three meanings — do not mix them.** `owner` is *who*: the module
  that owns a subscription, a journal, a decision. `tag` is *the mark a source is
  shown with* in a record (the module's `tag`, the log record's tag). `name` is
  *what it is called* (the module's name, a channel's name, a file's name). A
  field never borrows a neighbour's word: the log channel's admin prefix is a
  `name`, the subscription's holder is an `owner`.
- **A brick is added for a need, and looked at again afterwards.** When
  something is missing, add it: in the right place, with its own header, its
  own implementation and its own test. But nothing is added "for the sake of
  implementation" — every brick closes a concrete need. After it is in, look
  at the code around it once more: what was written inline and now has a
  brick of its own, what belongs to a neighbouring level, what can be moved
  out. A function that does two independent things — allocates and zeroes,
  scans and builds — is two bricks and one caller, never one function.
- Use `lh_fs_path_t` for private LH filesystem operations above `lh/os/system`;
  keep `authum_path_t` on the public Authum ABI. Raw native path text belongs at the kernel boundary. Preserve unresolved placeholder text as an Authum policy.
- Include project headers with angle brackets and namespace paths. Keep module example
  contracts in that module's include namespace, not in the Authum host API.
- **A variable is declared where it is used.** The code is C11, so a declaration
  lives in the innermost block that needs it, right before the statement that
  uses it — not in a pile at the top of the function (that is the C89 habit).
  A variable the whole body works with may stay at the top of the function.
  `python tools/check-decls.py <paths>` reports the piles (two or more
  declarations in a row that the next statement does not all use) and must
  report 0; `--fix` moves each plain declaration down to the statement that
  first uses it, joins it with its first assignment (`T x = ...;`) or puts a
  counter into its `for` header, and drops plain variables nobody uses. In C++
  it moves objects only with `--objects`, and lists their types: a constructor
  that does work elsewhere (a scope guard) keeps its place, with a comment line
  that says why.
- **A loop counter lives in the `for` header.** A variable that belongs to the
  loop alone is declared in the header itself: `for (lh_usize_t i = 0; i < 8u;
  ++i)`. A separate `lh_usize_t i;` line above the loop is right only when the
  variable outlives the loop and is read after it. Both forms compile to the
  same code — the difference is the scope and what the line says at a glance,
  not the speed.
- **A variable lives as long as the pointer that names it.** A declaration moves
  into the block that needs it only when nothing outlives that block. When the
  address of a variable is taken and the pointer is read after the block ends,
  the variable stays in the outer scope: the block ends, the variable is gone,
  and the pointer names nothing. `python tools/check-decls.py <paths>` reports
  that shape as well.
- **A pointer is checked once, at the boundary that receives it.** A public
  function checks the arguments it takes from the outside; an internal helper
  that only ever gets the pointer from a checked caller does not check it
  again. One path never checks the same pointer twice: when the caller checks
  and the callee checks too, one of the two goes — keep the check in the
  function that owns the contract (the public one), and drop the caller's.
  `python tools/check-asserts.py <paths>` reports the double checks.
- Preserve line endings when scripting edits; inspect the diff for accidental blank lines.
- Run `python tools/check-conventions.py` after foundation or include changes.
- Run `python tools/check-native.py`: Authum names no platform — no `_WIN32`, no
  libc, no native calls; the OS lives in LH. It reports the debt (see
  `docs/cleanup.md`) and must reach 0.
- Run `python tools/check-layout.py` after any code change: it must report 0
  places (top-level blocks separated by blank lines; `--fix` inserts them).
- A problem pointed out in one file is checked across both projects, not only there.

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

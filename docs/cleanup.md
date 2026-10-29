# Cleanup backlog

Open work for bringing Authum and LH in order, in the owner's priority.
LH comes first; Authum follows once LH provides the bricks.
Rules behind every item: [AGENTS.md](../AGENTS.md), [lib/lh/AGENTS.md](../lib/lh/AGENTS.md).
Close an item by deleting it; add new debt here instead of fixing it silently.

## LH (first)

- [ ] **Order every module by level** (AGENTS.md 0.0, `tools/order-levels.py`).
  The tool refuses a file whose definitions sit between `#if` blocks (it would
  move code in or out of them): `crypto/rijndael.c` (AES-NI) is ordered by hand or
  after its AES-NI part moves to a file of its own.
  Done: `memory/view` (10 levels), `memory/bounds`, `memory/typed` (9 levels),
  `str/view` (4), `wstr/view` (2).
- [ ] **Raw fields outside accessors** (AGENTS.md 0.0). `python tools/check-fields.py --own
  <files>` counts accesses to our own struct fields (native OS and HLSDK structs
  are not counted); `tools/gen-accessors.py` generates level-1 accessors.
  Done: the whole lower layer — `memory/*`, `str/*`, `array`, `str/list`,
  `wstr/list`, `http/*`, `spsc`, `logger`, `timer`, `math/*`, `numeric/big`,
  `fs/stat`, `real/*`, `sql/*`, `net/*`, `crypto/*`, `os/*`, most of `ui/*` (Authum `host/http/request.c` reads requests
  through them). Left in `ui` (~166): the tagged unions — `lh_ui_decl_t` (`row->as.label.text`),
  `lh_ui_web_x_t` (`x->u.dot.obj`), `lh_ui_web_html_t` — want accessors per variant,
  which the tools do not generate yet; the private run state of `ui/radius.c`;
  a few out-parameter addresses of value fields (`&out->y0`, `&run->spread`).
  In `os` only native structs (`fd_set`, `XImage`, `XEvent`, the `user32.h` mirrors, ...),
  which are not ours and stay. Open: the Xlib and `user32` import tables are called
  through raw members (`paint->x11->XPutImage`) — function tables like the MySQL client
  one, which got accessors; decide with the owner whether import tables get them too.
  `win/concurrency.c` includes `<windows.h>` where the rest of the Win32 backend uses
  its own mirrors.
  — and about 1400 in Authum (`host` ~770, `api` ~550,
  `gamedll` ~100 of our own structs).
- [ ] **The OS is the whole, the parts sit under it today** (AGENTS.md 0.1.1).
  The portable filesystem, network, threads, windows live under `lh/os/*`
  (`lh/os/fs/file.h`, `lh/os/net/...`); by the owner's concept they are parts
  (`lh/fs/...` beside the pure data already there) and `lh/os` assembles them.
  A large rename: plan the target tree with the owner first.
- [ ] **`lh/sql/mysql/api.h`:** 17 function-type typedefs, the table struct and
  its loader in one header (one file — one entity); `options` and
  `thread_end` declared by hand beside `lh_sql_mysql_api_fields`. The name
  `api` is to be discussed with the owner.
- [ ] **LH does not use its own bricks** (AGENTS.md section 0). Done: 131
  character literals → `lh/char/map.h` names (tests keep literals as data);
  `lh_ui_web_js_put_s64` → `lh_str_ptr_format_sllong`; the HTTP parser →
  `lh/char`, `lh_str_view_from_offset`, `lh_str_view_trim`,
  `lh_str_ptr_contains_char`. Left: sweep for hand-written digit loops,
  `switch` sets, min/max ternaries, open-coded trims and parses that have a
  brick; libc `str*` / `mem*` calls in portable code.
- [ ] **Audit for monoliths.** Split heavy functions into public bricks
  (decompose, delegate). Done: `lh/memory/allocator`. 64 functions exceed 60
  lines, 13 exceed 100. Worst: `lh_ui_web_page_build` (774, `ui/web/page.c`),
  `lh_os_system_window_open` (211, posix), `lh_os_system_win_window_proc` (207),
  `lh_ui_canvas_web_put_head` (156), `lh_memory_rfind_step` (92; `lh_memory_find_step` done),
  `lh_mb_link_poll` (155), `lh_memory_std_copy` (127), `lh_ui_decl_build` (114),
  `lh_str_ptr_format_float` (112), `lh_ui_web_host_ws_ingest` (105).
- [ ] **One file — one entity.** Done: 16 enums and 17 helper structs moved to
  their own headers (with their functions and sources where they had any).
  Left:
  - `net/modbus/link.h`: Modbus framing (`lh_mb_encode`, `lh_mb_decode`,
    `lh_mb_frame_length`, `lh_mb_rtu_frame_length`) and the link in one header
    and one source, in two extern "C" blocks.
  - `os/system/concurrency.h` (three handles + thread fn), `os/str.h`
    (char / str / view), `ui/frame/stats.h` (callback typedef → `stats/cb.h`).
  - The split headers copy every include of their parent; trim to what each
    one needs.
- [ ] **`static` functions: 61 in 11 files** (`ui/renderer3d.c`, `http/parser.c` done).
  `memory/std/x86/sse2.h` 12, `memory/std/words.h` 10, `memory/std/x86/avx2.h` 9,
  `crypto/rijndael.c` 8, `memory/std.c` 7, `os/worker.c` 5,
  posix/win concurrency, x11, cpuid. SIMD tiers may need target attributes —
  check before making them public bricks.
- [ ] **Naming pass.** Functions whose name does not say what they return
  (`get_` / `set_` / `is_` / `has_`) — in LH and in Authum.
- [ ] **Documentation briefs.** Replace one-line function comments with full
  contracts; every header gets a brief. Same form everywhere.
- [ ] **`lh_fs_path_t`.** It is awkward to use. Decide: simplify it, or split
  into two entities — a dynamic (owning) path and a static/fixed one usable
  directly with `lh_str_ptr`. Owner: "later", after the general cleanup.
- [ ] **Lines past 100 columns** (`.clang-format` ColumnLimit) in code nobody has touched
  since. Changed lines are formatted with clang-format 22 from MSYS2 clang64, changed lines
  only: `git-clang-format --binary <clang64>/clang-format.exe --style file --diff HEAD`.
- [ ] **Window tests are not in the LH test builds:** `build-lh-concurrency*` have
  `LH_LIBRARY_OPTION_OS_WINDOW=OFF`, so `os/window.c`, the backends and their tests only
  build in Authum's builds. `build-lh-window-windows` / `build-lh-window-linux` (ON) were
  added for this pass; make one of them the standard.
- [ ] **`_v` for the safe forms** of ui setters and getters (a check or a side effect) was
  chosen by the rule 'no suffix is the fast form'. Review with the owner, together with
  the lower-layer collisions the tools report: `lh_datetime_get_date` / `_get_time`,
  `lh_io_stream_get_reader` / `_writer`, `lh_io_dgram_*`, `lh_net_*_get_ip`,
  `lh_os_fs_file_set_path` take an out parameter or a pointer under an accessor's name.
- [ ] **Const containers hand out mutable elements.** `lh_list_get_first` / `_get_next`
  and `lh_array_get_ptr` take a const container and return a mutable node or slot, so
  everything built on them inherits the hole: `lh_os_module_event_bus_get_entry` / `_get_first` /
  `_get_next` / `_find_subscription` take a const bus and return writable records. Fix
  the bricks (a const getter and a `_mut` one), then their users.
- [ ] **`lh_test_os` is flaky under a parallel `ctest -j8`** (seen on Windows and on
  Linux): it passes on its own every time. Find the shared resource (a port, a timing).
- [x] **Every LH source mirrors a header** (`tools/check-mirror.py`, 0): `os/concurrency.c`
  split into `os/thread.c`, `os/mutex.c`, `os/condition.c`; the case tables declare in
  `util/{char,wchar}/case/fold.h` and `util/{str,wstr}/case/map.h`; the `memory/std`
  pieces are private headers beside `std.c` (`std/words.h`, `std/x86/*.h`), one
  translation unit as before.

## Authum (after LH)

- [ ] **Authum on LH rails: it does not know which OS it runs on.** No `#if _WIN32`,
  no native or libc call outside LH (owner, 2026-10-29). In order:
  1. [ ] LH local time: the zone offset of a UTC moment (DST included) in the OS
     backends, UTC ↔ local on top; `lh_os_timestamp_now` → `_try_get_now`.
  2. [ ] Logs (`host/logs.c`, `host/logs/files.c`, `writer.c`) on LH time, files and
     `lh/str/format`: no `time`/`localtime`/`strftime`/`fopen`/`snprintf`.
  3. [ ] HTTP on `lh/os/net/socket` + `lh/http`: one transport for every OS,
     `host/http_windows.c` (a "Linux only" stub) goes.
  4. [ ] The remaining libc calls in `api/` and `host/` (157 in 31 files at the start)
     → LH bricks; a missing brick is added to LH first.
  5. [ ] `gamedll/` and `include/authum/{export,symbol}.h`: calling convention, export
     and the engine library name through LH compiler / OS macros, no `_WIN32`.
  A check (`tools/check-native.py`) keeps it at 0.

- [x] **Console and commands become LH bricks.** `lh/console` (`lh_console_t`,
  commands, entries, calls, the reply sink) owns the registry; `host/commands.c` is a
  facade with the word length and the help line, and a module handler is the console
  handler itself (`authum_command_t` is laid out as `lh_console_call_t`, checked in
  `api/command.c`).
- [x] **`tools/gen-accessors.py` has an Authum mode.** A header outside `lib/lh` gets
  `void` setters and briefs with no LH symbol; `authum_command_def_t` has its accessors
  (`api/command/def.c`) and `host/commands.c` reads it through them.
- [ ] **Audit the rest of Authum for generic mechanisms**: tasks, threads, logs and
  their channels, cfg, placeholders, paths, the sandbox. Each is either generic
  (→ LH, Authum a thin facade) or the game (stays). Bring the list to the owner.
- [ ] **Rename the sandbox** (item 3 of the next task). It is hierarchical memory
  ownership with destructors on the LH ownership tree, not isolation, and the name
  misleads. Candidates: `arena`, `region`, `scope`; the owner decides.
- [ ] Authum has no named-character facade like `lh/char/map.h`; module code
  (`examples/chatfilter/chatfilter.c`) still compares bare literals.

- [ ] **Authum is a transparent facade over LH.** Inventory `include/authum/`:
  keep host-owned ABI (module/abi, host, events, commands, sandbox, engine,
  tasks/threads, shared HTTP service, log channels, placeholders); every
  generic brick (cfg, array, timer, logger, cstr, path, net address, json,
  sql, crypto) becomes a thin `authum_*` facade over the LH brick, with no
  logic of its own. Missing pieces are added to LH first.
- [ ] `api/net/address.c` parses host:port by hand → LH brick in `lh/net`.
- [ ] `cfg.c` → `lh/ini`; `array.c` → `lh_array_t`; `timer.c` → `lh/timer`;
  `logger.c` → `lh_logger_t`.
- [ ] `src/authum/api/memory.c` → delegate to `lh_runtime_allocator_try_alloc`,
  `_try_alloc_zero`, `_try_realloc` (now in LH; fixes the `LH_U32_T_MAX`
  overflow bound and the reach into LH allocator options).
- [ ] `gamedll/game_classes.cpp` (2176 lines) is generated by `tools/gen-game-classes.py`;
  consider having the generator emit one table instead of 195 functions.
- [ ] Code mixes libc (`strlen`, `strchr`, `memcpy`) with LH macros; move to
  LH kernels. Open question for the owner: `lh_math_add_one(p)` style for
  plain operators in Authum code — keep or relax.
- [ ] Docs: `docs/README.md` index is half table, half trailing lines;
  `logging-follow-up.md` and `docs/reports/` are working notes mixed with docs.
- [ ] Versions: CHANGELOG has 4.0.0 (2026-10-24) and no 3.0.0 — reconcile
  with the branch workflow.

# Cleanup backlog

Open work for bringing Authum and LH in order, in the owner's priority.
LH comes first; Authum follows once LH provides the bricks.
Rules behind every item: [AGENTS.md](../AGENTS.md), [lib/lh/AGENTS.md](../lib/lh/AGENTS.md).
Close an item by deleting it; add new debt here instead of fixing it silently.

## LH (first)

- [ ] **Order every module by level** (AGENTS.md 0.0, `tools/order-levels.py`).
  Done: `memory/view` (10 levels), `memory/bounds`, `memory/typed` (9 levels),
  `str/view` (4), `wstr/view` (2).
- [ ] **Raw fields outside accessors** (AGENTS.md 0.0). `python tools/check-fields.py --own
  <files>` counts accesses to our own struct fields (native OS and HLSDK structs
  are not counted); `tools/gen-accessors.py` generates level-1 accessors.
  Done: `memory/*`, `str/*`, `array`, `str/list`, `wstr/list`, `http/*`
  (and Authum `host/http/request.c` reads requests through them). Left: about
  3400 in LH — `ui` ~1950, `os` ~860, `crypto` 185, `net` 132, `sql` 65,
  `real` 48, root files (`spsc`, `logger`) 75, `math` 23, `numeric` 19,
  `fs/stat` 14 — and about 1400 in Authum (`host` ~770, `api` ~550,
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
  - `os/system/concurrency.h` (three handles + thread fn), `os/str.h`
    (char / str / view), `ui/frame/stats.h` (callback typedef → `stats/cb.h`).
  - The split headers copy every include of their parent; trim to what each
    one needs.
- [ ] **`static` functions: 61 in 11 files** (`ui/renderer3d.c`, `http/parser.c` done).
  `memory/std/x86/sse2.c` 12, `memory/std/words.c` 10, `memory/std/x86/avx2.c` 9,
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
- [ ] Pre-existing test failures: `lh_test_math` (`math/view.cpp` 149–153),
  `lh_test_ui` (`web_ws.length_127_roundtrip_over_64k`).

## Authum (after LH)

- [ ] Authum has no named-character facade like `lh/char/map.h`; module code
  (`examples/chatfilter/chatfilter.c`) still compares bare literals.

- [ ] **Authum is a transparent facade over LH.** Inventory `include/authum/`:
  keep host-owned ABI (module/abi, host, events, commands, sandbox, engine,
  tasks/threads, shared HTTP service, log channels, placeholders); every
  generic brick (cfg, array, timer, logger, cstr, path, net address, json,
  sql, crypto) becomes a thin `authum_*` facade over the LH brick, with no
  logic of its own. Missing pieces are added to LH first.
- [ ] `net_address.c` parses host:port by hand → LH brick in `lh/net`.
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

# Cleanup backlog

Open work for bringing Authum and LH in order, in the owner's priority.
LH comes first; Authum follows once LH provides the bricks.
Rules behind every item: [AGENTS.md](../AGENTS.md), [lib/lh/AGENTS.md](../lib/lh/AGENTS.md).
Close an item by deleting it; add new debt here instead of fixing it silently.

## LH (first)

- [ ] **Audit for monoliths.** Split heavy functions into public bricks
  (decompose, delegate). Done: `lh/memory/allocator`. 64 functions exceed 60
  lines, 13 exceed 100. Worst: `lh_ui_web_page_build` (774, `ui/web/page.c`),
  `lh_os_system_window_open` (211, posix), `lh_os_system_win_window_proc` (207),
  `lh_ui_canvas_web_put_head` (156), `lh_memory_find_step` (156),
  `lh_mb_link_poll` (155), `lh_memory_std_copy` (127), `lh_ui_decl_build` (114),
  `lh_str_ptr_format_float` (112), `lh_ui_web_host_ws_ingest` (105).
- [ ] **One file — one entity.** Done: 16 enums and 17 helper structs moved to
  their own headers (with their functions and sources where they had any).
  Left:
  - `os/system/concurrency.h` (three handles + thread fn), `os/str.h`
    (char / str / view), `ui/frame/stats.h` (callback typedef → `stats/cb.h`).
  - The split headers copy every include of their parent; trim to what each
    one needs.
- [ ] **`static` functions: 69 in 12 files** (`ui/renderer3d.c` done).
  `memory/std/x86/sse2.c` 12, `memory/std/words.c` 10, `memory/std/x86/avx2.c` 9,
  `crypto/rijndael.c` 8, `http/parser.c` 8, `memory/std.c` 7, `os/worker.c` 5,
  posix/win concurrency, x11, cpuid. SIMD tiers may need target attributes —
  check before making them public bricks.
- [ ] **Naming pass.** Functions whose name does not say what they return
  (`get_` / `set_` / `is_` / `has_`) — in LH and in Authum.
- [ ] **Documentation briefs.** Replace one-line function comments with full
  contracts; every header gets a brief. Same form everywhere.
- [ ] **`lh_fs_path_t`.** It is awkward to use. Decide: simplify it, or split
  into two entities — a dynamic (owning) path and a static/fixed one usable
  directly with `lh_str_ptr`. Owner: "later", after the general cleanup.
- [ ] **HTTP parser (`lh/http.h`).** Owner does not see why it is shaped this
  way; consider a separate module. Needs review and a decision.
- [ ] Pre-existing test failures: `lh_test_math` (`math/view.cpp` 149–153),
  `lh_test_ui` (`web_ws.length_127_roundtrip_over_64k`).

## Authum (after LH)

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
- [ ] `gamedll/game_classes.cpp`: 195 copy-pasted forwarders → one X-macro table.
- [ ] Code mixes libc (`strlen`, `strchr`, `memcpy`) with LH macros; move to
  LH kernels. Open question for the owner: `lh_math_add_one(p)` style for
  plain operators in Authum code — keep or relax.
- [ ] Docs: `docs/README.md` index is half table, half trailing lines;
  `logging-follow-up.md` and `docs/reports/` are working notes mixed with docs.
- [ ] Versions: CHANGELOG has 4.0.0 (2026-10-24) and no 3.0.0 — reconcile
  with the branch workflow.

# Cleanup backlog

Open work for bringing Authum and LH in order. The goal is an empty list: when the
last item is closed, this file is deleted. New features are not debt — they go to
[docs/roadmap.md](roadmap.md) (Authum) or [lib/lh/docs/roadmap.md](../lib/lh/docs/roadmap.md)
(LH). Rules behind every item: [AGENTS.md](../AGENTS.md), [lib/lh/AGENTS.md](../lib/lh/AGENTS.md).
Close an item by deleting it; what was done goes to the CHANGELOGs.

## LH and Authum together

- [ ] **Names say what they return.** A scan of public `lh_bool_t` declarations found
  493 candidates without `is_` / `has_` / `can_` / `needs_` / `try_`; a candidate is
  not a defect (getters and fallible actions are reviewed one by one, e.g.
  `equals` / `contains` / `parse` / `open`). Then the other functions whose name
  does not say what they return. Rename with every caller, test and document;
  backend-native API keeps its own conventions.
- [ ] **Duplicate pointer assertions** (LH AGENTS.md section 0). Audit by layer;
  inspect delegation and every execution path before removing one. 2026-10-31: 948
  heuristic candidates in 85 files, 50 cross-file tuples
  ([report](reports/lh-pointer-assertions-2026-10-31.md)); six removed in `str` /
  `wstr`. `tools/check-asserts.py` first needs to read multiline definitions and
  match parameter positions, not spellings.
- [ ] **Raw fields outside accessors** (AGENTS.md 0.0, `tools/check-fields.py --own`,
  `tools/gen-accessors.py`). LH is done but `ui` (98): the tagged union
  `lh_ui_decl_t` (accessors per variant), the run state of `ui/radius.c`, a few
  out-parameter addresses of value fields. The Xlib and `user32` import tables get
  accessors like the MySQL client table (owner, 2026-11-12); `win/concurrency.c`
  includes `<windows.h>` where the rest of the Win32 backend uses its own mirrors.
  Authum: about 1400 (`host` ~770 with `host/tasks.c`, `api` ~550, `gamedll` ~100).
- [ ] **Bricks that exist are used.** Left: hand-written digit loops, `switch` sets,
  min/max ternaries, open-coded trims and parses; libc `str*` / `mem*` in portable
  LH code and in Authum. A formatter followed by `dst[n] = nul`
  (`authum_cstr_format_u64` / `_i64`, `api/net.c`, `http/transport.c`,
  `http/request.c`): lh's number and IPv4 formatters need a `_cstr` form.
- [ ] **Lines past 100 columns** in code nobody has touched since; format with
  clang-format from MSYS2 clang64.

## LH

- [ ] **GCC 16 LTO warnings.** `-Wstringop-overread` in SQLite (`sqlite3Strlen30` in
  `sqlite3ColumnSetColl`, i686 Release, `LH_LIBRARY_OPTION_SQL_SQLITE=ON`);
  `-Wstringop-overflow` in the SHA-1 digest of the web host's WebSocket handshake and
  under `lh_str_ptr_format_ullong`; `-Wstringop-overflow` in `lh_memory_store_u64` /
  `lh_memory_std_rcopy_sse2` from `lh_str_ptr_format_real_fixed` (linking
  `authum_test_threads`). Reproduce each, then decide: a real short buffer is fixed,
  a false positive is silenced in the source (a size the compiler can see), never by
  dropping the warning.
- [ ] **AES on a Cortex-M4 is slow.** One AES-128 block takes 8 758 cycles to encrypt and
  13 690 to decrypt on an STM32F407 (FIPS-197 vector passes); a table-driven
  implementation on the same core takes about 600-1 000. Measure the portable path and fix
  it at the root, as the text path was.
- [ ] **`-Wtype-limits` in generic macros** on a bare-metal `-Wextra` build:
  `lh_math_is_negative` and `lh_interval_*_is_*_overflow` compare unsigned values with 0
  (`math/ops.h:192-210`, through `interval.h`, `deflate.c`, `gzip.c`, `chunked.c`,
  `allocator.c`, `typed.c`). Harmless, but every board build prints it.
- [ ] **Compression state machines.** The gzip header feed and the whole-buffer
  DEFLATE decoder still combine several transitions; split them into documented,
  tested bricks. The DEFLATE reader takes source bytes one at a time and retries an
  incomplete dynamic table; measure and fix the roots first.
- [ ] **Recursive traversal has level cycles** in every owning tree of `lh/web`:
  `clear` → `destroy` → `deinit` → `clear` (json, html, js nodes), the JSON reader
  and writer, `lh_web_html_writer_try_write`, the JS writer. Give recursion an
  explicit contract the level checker understands.
- [ ] **Order every module by level** (`tools/order-levels.py`). Done: `memory/view`,
  `memory/bounds`, `memory/typed`, `str/view`, `wstr/view`, `str/scanf/spec`.
  `crypto/rijndael.c` (definitions between `#if` blocks) is ordered by hand or after
  its AES-NI part moves to a file of its own.
- [ ] **The OS is the whole, the parts sit under it** (AGENTS.md 0.1.1). The portable
  filesystem, network, threads and windows live under `lh/os/*`; by the concept they
  are parts and `lh/os` assembles them. Next step: a plan of the target tree for the
  owner (owner, 2026-11-12), then the rename.
- [ ] **One file — one entity.** `lh/os/sql/mysql/api.h` (17 function typedefs, the
  table and its loader); `net/modbus/link.h` (framing and the link, two extern "C"
  blocks); `os/system/concurrency.h` (three handles and the thread fn); `os/str.h`
  (char / str / view); `ui/frame/stats.h` (callback → `stats/cb.h`). The split headers
  copy every include of their parent; trim them.
- [ ] **Monoliths.** 64 functions exceed 60 lines, 13 exceed 100. Worst:
  `lh_ui_web_player_page_create_play` (446; `create_fit` 350, `create_emit` 310),
  `lh_os_system_window_open` (211, posix), `lh_os_system_win_window_proc` (207),
  `lh_ui_canvas_web_put_head` (156), `lh_mb_link_poll` (155), `lh_memory_std_copy`
  (127), `lh_ui_decl_build` (114), `lh_str_ptr_format_float` (112),
  `lh_ui_web_host_ws_ingest` (105), `lh_memory_rfind_step` (92).
- [ ] **`_v` and accessor names.** Lower-layer functions under an accessor's name that
  take an out parameter or a pointer: `lh_datetime_get_date` / `_get_time`,
  `lh_io_stream_get_reader` / `_writer`, `lh_io_dgram_*`, `lh_net_*_get_ip`,
  `lh_os_fs_file_set_path`.
- [ ] **Const containers hand out mutable elements.** `lh_list_get_first` / `_get_next`
  and `lh_array_get_ptr` take a const container and return a mutable slot; the event
  bus inherits it. A const getter and a `_mut` one, then their users.
- [ ] **The formatter lacks `%e`, `%g`, `%o`, the `+` / space flags and `*`**; a game mask
  with them is logged as the mask (`gamedll/engine.cpp`). `%f` rounds the shortest
  digits, not the binary value (`0.125` → `0.13`, printf gives `0.12`).
- [ ] **`lh_fs_path_t` is awkward.** Simplify it, or split it into an owning path and a
  fixed one usable with `lh_str_ptr` directly.
- [ ] **Window tests are not in the LH test builds** (`LH_LIBRARY_OPTION_OS_WINDOW=OFF`
  in `build-lh-concurrency*`); make `build-lh-window-*` (ON) the standard.
- [ ] **Documentation briefs.** One-line function comments become full contracts;
  every header gets a brief.

## Authum

- [ ] **`authum_http` failed twice under heavy load** (2026-11-12, Windows): once a
  timeout, once a failed check, while a WSL build ran beside `ctest -j4`. 20 runs alone
  and 6 full parallel runs without the WSL build passed. Find the timing it depends on.
- [ ] **Authum is a transparent facade.** Inventory `include/authum/` and the rest of
  `host/` (tasks, threads, logs and channels, cfg, placeholders, paths, the region):
  each piece is either generic (→ LH, Authum a thin facade with no logic) or the game
  (stays). Bring the list to the owner.
- [ ] **No named characters for modules.** Authum has no facade like
  `lh/core/char/map.h`; `examples/chatfilter/chatfilter.c` compares bare literals.
- [ ] **`gamedll/game_classes.cpp`** (2176 lines, `tools/gen-game-classes.py`): have the
  generator emit one table instead of 195 functions.

# Cleanup backlog

Open work for bringing Authum and LH in order, in the owner's priority.
LH comes first; Authum follows once LH provides the bricks.
Rules behind every item: [AGENTS.md](../AGENTS.md), [lib/lh/AGENTS.md](../lib/lh/AGENTS.md).
Close an item by deleting it; add new debt here instead of fixing it silently.

## LH (first)

- [ ] **GCC 16 LTO warns inside SQLite.** The Windows i686 Release build with
  `LH_LIBRARY_OPTION_SQL_SQLITE=ON` reports `-Wstringop-overread` in
  `sqlite3Strlen30`, inlined into `sqlite3ColumnSetColl`. Investigate the
  upstream code and compiler diagnosis before changing the bundled amalgamation.

- [ ] **Declaration piles in gzip and chunked decoder tests.**
  `check-decls.py` reports 11 existing candidate blocks in
  `test/src/lh/core/compress/gzip.cpp` and `test/src/lh/core/http/chunked/decoder.cpp`;
  review lifetimes before moving declarations.

- [ ] **Finish decomposing compression state machines.** The streaming DEFLATE
  and gzip readers delegate individual phases to public tested operations. The
  gzip header feed and whole-buffer DEFLATE decoder still combine several
  transitions; split them into independently documented and tested bricks. The
  DEFLATE reader requests compressed source bytes individually and retries
  incomplete dynamic-table parsing; measure and improve the shared roots before
  optimizing call sites.

- [ ] **Recursive traversal has level cycles.**
  Every owning tree in `lh/web` has them. The node files (`json`, `html`, `js`):
  `clear` -> `destroy` -> `deinit` -> `clear`. `lh_web_json_reader_read_value` <->
  `read_object` / `read_array`; `lh_web_json_writer_write` <-> `write_object` /
  `write_array`; `lh_web_html_writer_try_write` <-> `try_write_children`;
  `lh_web_js_writer_try_write_statement` / `try_write_expression` through
  `try_write_bare`, `try_write_items`, `try_write_function`, `try_write_body`.
  Give recursive traversal an explicit layering contract so the level
  checker can verify declaration order without treating this dependency as an
  ordinary acyclic helper chain.

- [ ] **Review direct address operators outside LH C implementations.** The LH
  C sources were switched to `lh_addr_of`; headers, test code and Authum still
  need a syntax-aware audit. C++ references and bitwise operators are not
  address operations and must not be changed by a text replacement.

- [ ] **Boolean API names need a contract-by-contract audit.** A scan of public
  `lh_bool_t` declarations found 493 candidates without an `is_`, `has_`,
  `can_`, `needs_`, or `try_` component. This is a candidate count, not 493
  confirmed defects: getters and fallible actions need separate review. For
  example, predicates named `equals` / `contains` and fallible `parse` / `open`
  APIs must be checked against AGENTS.md, then renamed with all LH and Authum
  callers, tests and documentation. Preserve backend-native API conventions.

- [ ] **The web host carries an application inside it.** `lh_ui_web_host_t`
  serves a hardcoded Canvas2D player at `/` and answers a fixed route set
  (`/ui/press`, `/ui/release`, `/ui/move`, `/ui/wheel`, `/ui/key`, `/ui/text`,
  `/ui/size`, `/ui.json`, `/ws`); the player picks its own scale mode
  (`?scale=fit`, `layout` otherwise). The transport, the application and the
  protocol are one thing, and the scaling is a hidden default. Split them:
  the host is the transport, the application belongs to the caller (its page,
  its routes, its scaling), and a REST surface is a brick the caller attaches
  when it wants one — a desktop program has no use for it. Nothing is removed:
  the player becomes an application built on the transport. The target and the
  cost are in [lib/lh/docs/roadmap.md](../lib/lh/docs/roadmap.md) item 1.
- [ ] **There is no XML.** `lh/core/web/html` is an owning tree (node, attribute,
  escaping, names, writer), but there is no XML in lh at all. What is wanted is an entity (the
  data) and a node (the same entity with the operations), with HTML as XML
  under different tagging rules. Roadmap item 2.
- [ ] **There is no JS engine.** Scripts are built as a tree (`lh/core/web/js` node
  and writer; `src/lh/ui/web/player/page.c` builds the player with it) and
  only serialized to text; nothing in lh runs them.
  The engine is QuickJS (C, MIT, no dependencies, ES2025; MuJS and Duktape are
  ES5, V8 and SpiderMonkey are C++), built into the ecosystem on LH wrappers, not
  loaded from outside: its core (`quickjs.c`, `libregexp`, `libunicode`,
  `dtoa`) sits in `lib/` as third-party code, and `quickjs-libc.c` is not used.
  Memory goes through `JS_NewRuntime2` to the LH runtime allocator; output,
  files, time and the module loader are LH bricks (`lh/os/*`, `lh/core/str/*`,
  `lh_timer`). The rest of lh sees one brick of its own (`lh/core/web/js/engine`)
  that takes script text or precompiled bytecode (`JS_WriteObject` /
  `JS_ReadObject`), so an application serializes to text and still runs on a
  bytecode machine. The core's own libc calls go through LH wrappers too (owner,
  2026-11-02); a missing wrapper is added to LH first. Roadmap item 3.
- [ ] **The gamepad has an event type and no driver.** `lh_os_input_kind_controller_button_down`,
  `_controller_axis` and the device kind `controller` exist, and no backend
  produces them: no XInput on Windows, no evdev on Linux, no device
  enumeration, no hot-plug. The mouse, keyboard, touch and pen have real
  backends; the gamepad is the gap. Roadmap item 4. The desktop applications
  are the consumers — the Authum game modules are server side, so the input
  never crosses the Authum facade.
- [ ] **Duplicate pointer assertions** (LH AGENTS.md section 0). Audit LH
  and Authum by layer; inspect delegation and every relevant execution path
  before choosing which assertion to remove. A public wrapper may rely on a
  checked brick only before its first use of the pointer; a private helper
  may rely on all its callers. Keep independently callable brick contracts.
  `tools/check-asserts.py` is a same-file heuristic, not proof or a complete
  inventory. Audit 2026-10-31: 948 call-site candidates in 85 files / 316 caller
  functions, plus 50 supplemental cross-file tuples (all heuristic). Full
  inventory and reviewed examples: [report](reports/lh-pointer-assertions-2026-10-31.md). Start with memory, then strings, remaining
  foundations, network and UI; verify each affected layer and integration.
  First implementation pass (2026-10-31): reviewed the memory move branches
  and string candidates; removed six delegated assertions in `str` / `wstr`
  (`as_view`, `truncate`, `list_get`). The 948-checker baseline does not fall:
  these chains cross files or pass the pointer outside the first argument.
  Retained parser, scanf conversion/default, formatter early-return and
  memory tiny/SIMD guards. The rest of the inventory remains open.
- [ ] **`lh_str_scanf_spec_clear` dereferences `self` before its setter checks it.**
  `str/scanf/spec.c` assigns a zero struct directly; the setter assertion is
  too late. Bring clear onto the accessor bricks and enforce its contract
  before the write. Found during the pointer assertion pass.
- [ ] **`lh_wstr_view_init` and `lh_wstr_view_lit` disagree on the empty string.**
  `lh_test_str` fails on `wstr_view_make.empty_string_matches_the_literal_spelling`:
  the runtime spelling of `L""` yields other endpoints than the literal one.
  Since `fcc91005` (2026-10-28) `lh_wstr_view_init` no longer stores the
  endpoints of an empty string itself, it goes through
  `lh_wstr_view_init_by_size`, which rejects the size and falls back to an
  explicitly empty view. The two spellings must agree: "present and empty"
  must not depend on which one the caller wrote. Found on 2026-10-31 while the
  declaration pass ran the LH tests; the failure predates that pass.
- [ ] **GCC 16 reports a false `-Wstringop-overflow` in the SHA-1 path.**
  In the LTO link of the LH tests: "at offset 63 into destination object
  'digest' of size 20" (`ui/web/host.c`, the WebSocket handshake). The write
  is bounded — `lh_crypto_sha_finish` returns 0 when the digest size does not
  fit, and the caller passes exactly `LH_CRYPTO_SHA1_DIGEST_SIZE` bytes — but
  GCC cannot prove the digest size of the kind at the LTO stage, so the loop
  in `lh_crypto_sha_put_digest` looks unbounded to it. The same noise appears
  for `lh_memory_std_set` under `lh_str_ptr_format_ullong`. Silence it in the
  source (a size the compiler can see), not by dropping the warning.
  The formatter now checks the chunk length and remaining capacity explicitly;
  GCC 16.2 i686 and 16.1 x86-64 Release/FAT LTO did not reproduce its warning
  even on the original source. Keep this open until the original configuration
  reproduces it and confirms the fix.
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
  through them). Left in `ui` (98): the tagged union `lh_ui_decl_t` (`row->as.label.text`)
  wants accessors per variant,
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
  (`lh/core/fs/...` beside the pure data already there) and `lh/os` assembles them.
  A large rename: plan the target tree with the owner first.
- [ ] **`lh/os/sql/mysql/api.h`:** 17 function-type typedefs, the table struct and
  its loader in one header (one file — one entity); `options` and
  `thread_end` declared by hand beside `lh_os_sql_mysql_api_fields`. The name
  `api` is to be discussed with the owner.
- [ ] **LH does not use its own bricks** (AGENTS.md section 0). Done: 131
  character literals → `lh/core/char/map.h` names (tests keep literals as data);
  `lh_ui_web_js_put_s64` → `lh_str_ptr_format_sllong`; the HTTP parser →
  `lh/char`, `lh_str_view_from_offset`, `lh_str_view_trim`,
  `lh_str_ptr_contains_char`. Left: sweep for hand-written digit loops,
  `switch` sets, min/max ternaries, open-coded trims and parses that have a
  brick; libc `str*` / `mem*` calls in portable code.
- [ ] **Audit for monoliths.** Split heavy functions into public bricks
  (decompose, delegate). Done: `lh/core/memory/allocator`. 64 functions exceed 60
  lines, 13 exceed 100. Worst: `lh_ui_web_player_page_create_play` (446, `ui/web/player/page.c`; also
  `create_fit` 350, `create_emit` 310),
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
- [x] **`static` functions** — none left, LH and Authum (2026-11-11; see lh's CHANGELOG
  for the A/B measurement of the memory kernel). Was: **55 in 9 files** (`ui/renderer3d.c`, `http/parser.c`,
  `os/worker.c`, and on 2026-11-11 `os/mutex.c` / `thread.c` / `condition.c` done —
  `lh_os_fail`). `memory/std/x86/sse2.h` 12, `memory/std/words.h` 10,
  `memory/std/x86/avx2.h` 9, `crypto/rijndael.c` 8 (AES-NI), `memory/std.c` 7,
  posix/win concurrency 5, x11 3, cpuid 1. SIMD tiers may need target attributes —
  check before making them public bricks.
- [ ] **Brick-bypass audit, 2026-11-11** (owner: "do we use our wrappers everywhere?").
  A scan of LH and Authum (third-party sqlite and the native backends left out). Done:
  min/max, swap, the lexer's `'\0'`, the `_invalid` statics (see CHANGELOG). Left:
  - [x] **C-style casts** (done 2026-11-11): 275 `(T)x` → `lh_cast_static(T, x)` in LH
    and Authum sources (`curve25519/field.c` 224, `ed25519`, `ui`, Authum's `runtime.c`,
    `tasks.c`, `file.c`, `cstr.c`, `timer.c`). Left on purpose: `(void)x` (the discard
    idiom) and the public `include/authum/` headers, which may not use lh.
  - [x] **Text as `(const char *, size)`** (done 2026-11-11):
    `lh_os_module_event_entry_try_create` / `_create` take an `lh_str_view_t`;
    `authum_http_policy_contains` takes the value as a view (its callers cut the
    directory and the extension with `lh_str_view_from_offset`);
    `authum_http_parse_key` had no caller and is gone; `authum_placeholder_find_standard` /
    `_is_standard` take an `lh_str_view_t` (the lookup already held one). Kept:
    `authum_http_is_terminated(buffer, capacity)` — a buffer and its capacity, not text.
  - [x] **Hand-NUL-terminated copies** (done 2026-11-11): the brick is
    `lh_str_ptr_copy_cstr`; the JS lexer, the web player's page, `cfg.c`,
    `authum_cstr_copy`, `json.c`, `path.c`, `sql.c`, `http/limits.c` use it. Left on
    purpose: loops that rewrite characters while copying (`engine.c`, `http/access.c`,
    `logs.c`, `ui/label.c`), the overlapping move in `net/address.c`, and the
    terminators of the bricks themselves (`str/format/text.c`, `util/str/ptr.c`).
    Still open: a formatter followed by `dst[n] = nul` (`authum_cstr_format_u64` /
    `_i64`, `api/net.c`, `http/transport.c`, `http/request.c`) — lh's number and IPv4
    formatters have no `_cstr` form; the text formatter has one.
  - [x] **Authum character literals** (33), done 2026-11-11: `api/time.c` formats with
    `lh_datetime_format_by` + `lh_str_ptr_format_cstr`; `host/http/resources.c` tests
    the reserved name characters and the end of the path with `lh_char_map_*` sets
    (`lh_str_ptr_contains_char` / `_find_of_chars`), decodes `%XX` with
    `lh_str_ptr_parse_hex_bytes` and digits with `lh_char_is_digit`;
    `host/http/request.c` reads an all-zero Content-Length with
    `lh_str_ptr_find_not_of_chars`; `api/cstr.c` / `api/events.c` name their characters.
  - [x] (done 2026-11-11: `lh_char_to_upper` / `lh_char_to_lower`, `lh_char_is_ascii`;
    `authum_char_to_upper` is one call.) Was: `authum_char_to_upper` upper-cases one
    character through a two-byte buffer and
    `lh_str_ptr_to_upper`: lh's one-character mapping (`lh_str_ptr_map_upper_one` /
    `_lower_one` in `util/str/case/map.c`) is hidden and exists only under the table
    option. Make `lh_char_to_upper` / `lh_char_to_lower` public bricks in
    `lh/core/char/`, the string forms built on them.
  - [x] `host/http/resources.c`'s URL-path check is in lh (2026-11-11):
    `lh_http_target_try_copy_path`, `lh_http_is_name_char`; Authum keeps its policy
    (`deny_prefixes`, `allow_dirs`, extensions) and logs the same reason names.
  - Copies that rewrite characters on the way (`api/engine.c` blanks `" ; CR LF`,
    `host/http/access.c` keeps visible ASCII but `"` `\`, `host/logs.c` keeps word
    characters, `-` and `.`): left as loops on purpose (owner, 2026-11-11). A shared
    brick would take a predicate callback, and each caller would then need a public
    predicate function of its own (no `static`) — more surface for no gain.
- [ ] `lh_test_os` failed once under `ctest -j8` on Windows (2026-11-11) and passed
  alone and in two full runs after; probably a socket test sharing a port with a
  parallel one. Find the test and give it its own port.
  - [x] `ui/shadow/shape.c`: the zero-fill loop is `lh_memory_set`, `|px - cx|` is
    `lh_math_abs` (2026-11-11).
  - [x] **Plain operators** (`==`, `+`, `++`): decided by the owner (2026-11-11) —
    operators are fine, a wrapper that is only the operator is optional, existing
    ones stay; bricks with a meaning are required (AGENTS.md, "Operators are not
    bricks; meanings are").
  - [ ] **`!p` on pointers** → `lh_ptr_is_null(p)` / `authum_null_eq(p)` (the same
    rule). LH ~886 and Authum ~457 `!x`, but that count mixes pointers with flags
    (`!lh_x_is_y()`); a pass has to tell them apart by hand, file by file.
- [ ] **Naming pass.** Functions whose name does not say what they return
  (`get_` / `set_` / `is_` / `has_`) — in LH and in Authum.
- [ ] **Documentation briefs.** Replace one-line function comments with full
  contracts; every header gets a brief. Same form everywhere.
- [ ] **`lh_fs_path_t`.** It is awkward to use. Decide: simplify it, or split
  into two entities — a dynamic (owning) path and a static/fixed one usable
  directly with `lh_str_ptr`. Owner: "later", after the general cleanup.
- [ ] **Declaration piles in the string layer.** The assertion pass reports
  13 existing places across `str.c`, `wstr.c`, `str/list.c`, `wstr/list.c`
  (`check-decls.py`); keep the declaration cleanup as a separate pass.
- [ ] **Lines past 100 columns** (`.clang-format` ColumnLimit) in code nobody has touched
  since. Changed lines are formatted with clang-format 22 from MSYS2 clang64, changed lines
  only: `git-clang-format --binary <clang64>/clang-format.exe --style file --diff HEAD`.
- [ ] **Window tests are not in the LH test builds:** `build-lh-concurrency*` have
  `LH_LIBRARY_OPTION_OS_WINDOW=OFF`, so `os/window.c`, the backends and their tests only
  build in Authum's builds. `build-lh-window-windows` / `build-lh-window-linux` (ON) were
  added for this pass; make one of them the standard.
- [ ] **`_v` for the safe forms** of ui setters and getters (a check or a side effect).
  The rule is the owner's (2026-10-30): the fast form has no suffix and does not
  check, the validating form has `_v`; `_unsafe` is gone from LH (the four pointer
  arithmetic macros of `lh/core/util/ptr.h` are now `lh_ptr_{add,sub}[_by_offset][_v]`).
  Left: the lower-layer collisions the tools report — `lh_datetime_get_date` /
  `_get_time`, `lh_io_stream_get_reader` / `_writer`, `lh_io_dgram_*`,
  `lh_net_*_get_ip`, `lh_os_fs_file_set_path` take an out parameter or a pointer
  under an accessor's name.
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

- [ ] **Assertion checker misses multiline definitions and cross-file calls.**
  `tools/check-asserts.py` recognizes only a signature whose function-name
  line is followed by `{`, and matches variable spelling rather than actual
  parameter positions. Extend its analysis before using its count as a
  cleanup completion criterion; retain manual control-flow review.

## Authum (after LH)

- [ ] **Generic mechanisms of Authum move to LH** (owner, 2026-11-10), in this order:
  7. [x] `api/net.c` IPv4 classes → `lh/core/net/ip` predicates
     (`lh_net_ip4_is_public`, `lh_net_ip4_to_u32`, `lh_net_ip4_init_by_u32`).
  6. [x] `api/logger/buffer.c` capture of a thread's lines → `lh/os/logger/capture`
     (`lh_os_logger_capture_t`); Authum keeps the channel as the line's target and
     the overflow warning.
  3. [x] `host/placeholders.c` `{name}` expansion and its owned registry → `lh/core/str/template`
     (`lh_str_template_try_expand`, `lh_str_template_registry_t`); Authum keeps the
     standard names, canonical absolute values and the module-relative rule.
  2. [x] `host/hooks.c` interception chain → `lh/os/module/hook`, beside the event bus
     (`lh_os_module_hook_t`); Authum keeps the points, the 5 ms budget and the verdict.
  4. [x] `host/http/limits.c` per-IP limits, token bucket, trusted proxies → lh
     (`lh/core/net/cidr`, `lh/core/http/forwarded`, `lh/core/http/limit/{bucket,peer}`);
     Authum keeps the timeouts, the refusal summary and its address keys.
  1. [x] `host/loader.c` module discovery and the list file with OS sections → lh
     (`lh/os/loader/{image,scan,list}`, `lh_os_system_get_name`, `lh_os_fs_is_file` /
     `_is_dir`, `lh_os_fs_file_read_all(_by_path)`, `lh_fs_path_try_get_name`);
     Authum keeps `modules/` and `configs/modules.list` beside its image, the log
     lines and the refusal to load `authum` itself.
  5. [x] `api/sql/thread.c` one connection per task thread → lh
     (`lh/core/sql/address`, `lh_os_sql_address_try_open`, `lh/os/sql/keep`,
     `lh_str_is_equal`); an `authum_sql_t` borrows the kept connection
     (`authum_sql_borrow`). Authum keeps the module's description and its defaults,
     the result and the end-of-thread registration.
  Authum keeps the game's names and policy in each.
- [ ] **Declaration piles left in older functions** (2026-11-11).
  `tools/check-decls.py` reports 10 in `lib/lh/src/lh/core/fs/path.c` (`get_segment`,
  `join`, `parent`, ...), 3 in `lib/lh/src/lh/core/str.c` (lines 68, 113, 159) and 1 in
  `src/authum/api/cfg.c:124`; they predate the rule and were seen while adding
  `lh_fs_path_try_get_name` and `lh_str_is_equal` next to them.
- [x] **LH has no cutting format into a fixed buffer** (2026-11-10, done 2026-11-11).
  The engine has a cut mode (`lh_str_ptr_format_text_walk(..., cut)`,
  `lh_str_ptr_format_text_get_fit`); `lh_str_ptr_format_text_cut_v` and
  `lh_str_ptr_format_cstr_cut_v` use it, and so do `lh_os_logger_capture_try_push_v`
  and `authum_cstr_format_v` — no allocation per line any more.
- [x] **An SQL failure is text only** (owner's question, 2026-11-11; done the same
  day in lh: `lh_sql_error_kind_t`, `lh_sql_db_get_error_kind` / `_get_error_code`,
  `lh_sql_db_fail_as`, `lh_os_sql_mysql_get_error_kind`, `lh_os_sql_sqlite_get_error_kind`;
  every backend failure sets them).
- [ ] **Modules see only the text of an SQL failure.** `authum_sql_thread_result_t`
  and `authum_sql_t` are frozen, so the kind and the code reach a module only through
  new API (a getter on `authum_sql_t`, a new result structure): the owner decides
  when (a minor ABI step).
- [x] **`authum_logger_t` fields are raw** (2026-11-10, done 2026-11-11). Level-1
  accessors in `<authum/logger.h>` (`authum_logger_get_module_channel`, `_get_journal`,
  `_owns_journal`, ...); `api/logger.c` and the tests go through them, with
  `authum_logger_clear` / `_adopt_journal` (`api/logger/private.h`) above them.
- [x] **The host's log channel record is raw** (done 2026-11-11). `authum_logs_channel_t`
  moved to its own `host/logs/channel.h` with level-1 accessors (`host/logs/channel.c`)
  and `authum_logs_channel_init`; `host/logs.c` goes through them (about 90 places),
  the registry walk in `authum_logs_free` keeps the previous channel instead of a
  pointer into a field.
- [x] **The next module ABI break** (owner, 2026-11-10; called "ABI 3.0" then, but 3.x
  shipped with Authum 4.0.0, so it is **4.0**): the sandbox becomes `authum_region_t` (an
  ownership tree of memory, not isolation), and a module's files leave `FILE *`; all in
  one release, so modules are rewritten once. Done 2026-11-11, `AUTHUM_MODULE_ABI`
  4.0.0: the opaque `authum_array_t`; `authum_sandbox_*` → `authum_region_*`
  (`<authum/region.h>`, `authum_host_get_region`, docs/region.md); no public header
  hands out `FILE *` or includes `stdio.h`, and `path.h` points at `authum_file_open`
  instead of `fopen`. The RASPAD modules (outside this repository) move to 4.0 next:
  `sandbox` → `region`, `fopen` → `authum_file_t`.
- [ ] `AGENTS.md` says the module ABI is frozen at 2.0 (Authum 0.4.0); it is 4.0 now
  (Authum 4.1.0). The owner's text — the owner updates it.
- [ ] **LTO warns in lh's formatter** (seen 2026-11-11, mingw32 full rebuild):
  `-Wstringop-overflow` in `lh_memory_store_u64` / `lh_memory_std_rcopy_sse2`, inlined
  from `lh_str_ptr_format_real_fixed` → `lh_str_ptr_format_text_get_value` →
  `lh_str_ptr_format_text_walk.constprop` when `authum_test_threads` links. Tests pass;
  find whether GCC sees a real short buffer or a false positive, and fix it in lh.

- [ ] **`host/tasks.c` — сырые поля и static** (2026-11-04, шаг 4b исполнения).
  Файл переехал на `lh/rail`, но сохранил стиль соседей (`host/threads.c`):
  `authum_task_worker_t` / `authum_task_record_t` трогаются напрямую, колбэки и
  поиск владельца — `static`. По AGENTS нужны геттеры/сеттеры уровня 1 и
  публичные кирпичи; сделать вместе с остальным `host/`.

- [ ] **Кто это сделал: выходы без метки источника** (аудит 2026-10-30).
  Логи и команды — эталон: owner → канал → tag, всё через одну точку
  (`host/logs.c`, `host/commands.c` + LH console). Сделано: **игровые выходы
  помечены** — `authum_host_t` несёт метку модуля (`tag`), и всё, что модуль
  печатает (игроку, в чат, в консоль, причину кика), уходит с ней:
  `[WELCOME] Welcome, Vega!`. Не маркируется:
  1. **Publish событий**: владелец доходит до хоста, но выбрасывается
     (`host/events.c`, `authum_unused(owner)`) — сделано: модуль объявляет
     только своё имя (префикс проверяет хост, `api/events.c`), так что издатель
     — это владелец имени, а не соглашение.
  2. **Решения модулей** — сделано: решение живёт только в цепочке хуков, а
     хост сам пишет автора в вызов (`authum_hook_call_t.decided_by` — метка
     модуля, чей обработчик остановил вызов). Модуль не может решить и «не
     подписаться»: в логе `client <name> refused by <tag>`.
  3. **HTTP access-log** — сделано: строка называет модуль-владельца
     (`module=TESTMOD`), метка идёт от `authum_module_info_t.tag` через
     HTTP-владельца.
- [x] **Кто опубликовал событие: forward и query разделены.** `authum_events_forward`
  объявляет своё (имя обязано начинаться с префикса модуля — `scores.` для
  `scores`), `authum_events_query` спрашивает чужое (отвечает владелец имени).
  Одной функции больше нет: модуль не может объявить своё под чужим именем.
  Префикс хост передаёт модулю при загрузке (`authum_events_set_prefix`).
- [x] **Хуки для модулей** — дизайн в `docs/hooks.md`, код — в
  `src/authum/host/hooks.c` и `include/authum/hook.h`. Цепочка в ядре (порядок
  LIFO, бюджет 5 мс на обработчик, снятие при выгрузке модуля), API модуля
  (`authum_hooks_add` / `_remove`) и четыре точки: client connect, client kill,
  client command, client put in server. Отказ клиента, глотание команды и
  запрет убийства переехали из полей события (`allow`, `reject_reason`,
  `block`) в цепочку: одна дорожка к решению.
- [x] **Решения модулей — enforcement.** Решение живёт в цепочке, и хост сам
  пишет в вызов метку модуля, чей обработчик остановил его
  (`authum_hook_call_t.decided_by`). Модуль не может решить и промолчать: в
  логе всегда видно, кто решил. Отдельная структура «решение» не понадобилась
  — цепочка и есть объект решения.
- [ ] **`authum_module_info_t.logtag` → `tag`**: сделано (2026-10-30): поле
  переименовано, хост его читает и передаёт модулю (`authum_host_t.tag`), по
  нему идёт метка в логах и в цепочке хуков. ABI модуля 3.0.
- [ ] **Файлы модуля**: `authum_cfg_open` больше не идёт прямо в stdio (читает через
     `lh/os/fs/file.h`), но owner у файла модуля всё ещё нет; записи файлов у модуля
     нет вообще.
  Самое дешёвое: (1) вернуть owner в publish (нужна новая структура — ABI 2.0
  не растёт); (2) копия engine-таблицы на модуль + префикс `[модуль]` в текст
  вывода, как это уже делает зеркало лога; (3) отдельная структура «решение»
  с автором — не понадобилась, решение живёт в цепочке хуков; (4) добавить
  owner в access-строку — сделано.
- [ ] **`authum_detour_attach` — не мёртвый, но чужой механизм**: в Authum его
  никто не зовёт, однако `raspad-qproto` патчит им код движка
  (`Steam_NotifyClientConnect`: 5 байт `jmp rel32`, вызов оригинала через
  suspend/resume). Цепочка хуков его не заменяет — она подменяет таблицы
  `DLL_FUNCTIONS` / `NEW_DLL_FUNCTIONS`, а это правка кода движка. Решение:
  оставить как отдельный инструмент (патч кода) и не смешивать с хуками;
  когда qproto переедет на новый ABI — пересмотреть, нужен ли он в публичном
  API Authum.
- [ ] **Реестра сервисов нет**: модуль не может получить API другого модуля —
  единственный межмодульный канал шина по именам. Осознанно (пока) — но это и есть
  ответ на «API для API»: сначала метка источника, потом сервисы.
- [ ] **Точка решения «пускать / не пускать»** — заглушка (`host/gate.c`, план на
  1.0.0). Владелец точки назначается конфигом; остальные модули наблюдают.
- [x] **`tools/check-native.py`** — написан (2026-10-30): запрет `_WIN32`, libc и
  нативных вызовов в `src/authum`, `include/authum` и `examples`. Правило
  «Authum не знает про ОС» теперь проверяется, а не держится на слове.
- [ ] **Скриптовый слой (cnor).** Дизайн в `docs/scripting.md`: мост — обычный
  модуль Authum, который встраивает VM cnor и отдаёт ему таблицу нативов;
  ядро о скриптах не знает. Осталось: модуль-мост (VM + таблица нативов),
  путь решения (мостик вешает C-хук, скрипт отвечает), стандартная
  библиотека скриптов (`authum.cn`, `rules.cn`).
- [ ] **Authum on LH rails: it does not know which OS it runs on.** No `#if _WIN32`,
  no native or libc call outside LH (owner, 2026-10-29). In order:
  1. [x] LH local time: the zone offset of a UTC moment (DST included) in the OS
     backends, UTC ↔ local on top; `lh_os_timestamp_now` → `_try_get_now`.
  2. [x] Log files and queue on LH: `lh/os/logger/{record,sink,queue,daily}`; Authum's
     `host/logs/writer.c` and `files.c` are gone, `host/logs.c` sends records.
     `host/logs.c` has no libc call left (`lh/core/str/format`, `lh/os/env`, the runtime
     logger).
  3. [x] HTTP on LH handles and bricks: one transport for every OS, the "Linux only"
     `host/http_windows.c` stub is gone, its tests run on Windows too. Static files are
     looked up below each root through `lh_os_fs_file_try_open_read_at`, backed by
     handle-relative system operations on Windows and POSIX. The Windows backend
     uses `NtCreateFile` relative to each directory and refuses reparse points.
  4. [x] The remaining libc calls in `api/` and `host/` (157 in 31 files at the start)
     → LH bricks; a missing brick is added to LH first.
     `tools/check-native.py` now exists and reports the debt: **0 places**
     (the last 15 were closed 2026-10-30, all in `gamedll/`): 9 string calls
     (`std::strlen` / `std::strcmp` / `std::strchr` → `lh/core/str/*`), 3 clock calls
     (`lh/os/timestamp.h`), 2 platform branches (the LH compiler / OS macros),
     1 `getenv` (`lh/os/env.h`).
     The format, the files and the headers are done: `snprintf` →
     `lh/core/str/format/text.h` (`lh_str_ptr_format_cstr`, `authum_cstr_format`), the
     file calls → `lh/os/fs/file.h` (`api/cfg.c`, `api/env.c`, `gamedll/game.cpp`),
     the forbidden headers are gone. Done in this order: clock/env → the strings
     of `gamedll/`.
     `include/authum/export.h` and `include/authum/symbol.h` are exempt on
     purpose: they sit on the ABI boundary with the host process.
  5. [x] **The thread test does not run on Windows.** Переписан (2026-10-30) на
     кирпичи LH (`lh/os/condition.h`, `lh/os/mutex.h`): каждый воркер ждёт
     **своего** пробуждения на своём условии, дальше барьер. Сигнальная часть
     (`pthread_sigmask`, `SIGPIPE`) осталась POSIX-only, на Windows добавлен
     тест, что флаг `SIGPIPE` принимается и маскировать нечего. Тест идёт на
     обеих платформах.
- [ ] **lh formats no `%e`, `%g`, `%o`, `+`/space flags or `*`.** Length modifiers,
  `%f` (over `lh_str_ptr_format_real_fixed`) and `%p` are in. A game mask with what is
  left is logged as the mask itself (`gamedll/engine.cpp`). `%f` rounds the shortest
  digits, not the exact binary value (`0.125` → `0.13`; printf gives `0.12`).
- [ ] **`gamedll/` and `include/authum/{export,symbol}.h`: calling convention, export
     and the engine library name through LH compiler / OS macros, no `_WIN32`.
  A check (`tools/check-native.py`) keeps it at 0.
  Done for `gamedll/` (2026-10-30): the calling convention and the export go
  through `LH_COMPILER_OS` / `LH_COMPILER_TYPE` / `LH_COMPILER_ARCH`.
  Left: `include/authum/symbol.h` names the engine library per platform — a
  public Authum header may not include LH, and the platform is named there on
  purpose (the exemption above).
- [ ] **The module's own files went through libc.** `include/authum/cfg.h` hands a
  module a `FILE *` (`authum_cfg_t.file`, `authum_cfg_open_file`) and
  `include/authum/path.h` invites it to open files with `fopen`; the RASPAD
  modules do exactly that (fastdl, gatekeeper, monitoring, qproto — about 15
  places). The public `authum_file_t` with `authum_file_open/read/write/close`
  is a thin facade over `lh/os/fs/file.h` and is in place. Left: `authum_cfg_t`
  keeps its own file type instead of `FILE *`, `stdio.h` leaves the public
  headers, and the modules move over — after Authum is pushed, so they pick
  the new ABI 3.0 once.
- [x] **`lh/os/loop`: one worker loop under every background thread.** The log
  queue, `lh/os/worker` and Authum's HTTP worker run on it (HTTP with a waiter:
  `poll` on its sockets and an `lh/os/wake` signal); Authum's module threads are an
  `lh/os/thread/group` (their body is the module's own code, not a step).

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
- [x] **Rename the sandbox** (item 3 of the next task). It is hierarchical memory
  ownership with destructors on the LH ownership tree, not isolation, and the name
  misleads. Candidates: `arena`, `region`, `scope`; the owner decides. The owner chose
  `region` (2026-11-10); renamed 2026-11-11 with module ABI 4.0.
- [ ] Authum has no named-character facade like `lh/core/char/map.h`; module code
  (`examples/chatfilter/chatfilter.c`) still compares bare literals.

- [ ] **Authum is a transparent facade over LH.** Inventory `include/authum/`:
  keep host-owned ABI (module/abi, host, events, commands, sandbox, engine,
  tasks/threads, shared HTTP service, log channels, placeholders); every
  generic brick (cfg, array, timer, logger, cstr, path, net address, json,
  sql, crypto) becomes a thin `authum_*` facade over the LH brick, with no
  logic of its own. Missing pieces are added to LH first.
- [x] `authum_array_t` keeps its own storage: it is a public struct a module fills, and
  its memory comes from the module's sandbox, while `lh_array_t` owns a block of the
  runtime allocator. Its growth and overflow are lh's now (`lh_array_get_grown_capacity`,
  `lh_interval_closed_is_mul_overflow`). Full delegation needs an `lh_array_t` over a
  caller's allocator (the sandbox is one) — then ABI 3.0 can hide the struct. Done
  2026-11-11: `lh_array_init_by` takes an allocator (the `_by` forms of
  `lh/core/memory/bounds|typed/allocated`), and `authum_array_t` is opaque
  (`authum_array_create` / `_destroy`) — an lh array over the sandbox. Module ABI 4.0.0.
- [x] `cfg.c` reads its lines with lh's bricks (`lh_ini_parser_is_skipped`, views); a
  whole-file `lh/ini` read waits for sectionless `key = value` in lh/ini. Done
  2026-11-11: lh's parser has a global section (pairs before the first header; no
  mode, the format's reader decides about sections), and `authum_cfg_get_value` reads
  the text with it, passing over headers and bad lines as before.
- [ ] `gamedll/game_classes.cpp` (2176 lines) is generated by `tools/gen-game-classes.py`;
  consider having the generator emit one table instead of 195 functions.
- [ ] Code mixes libc (`strlen`, `strchr`, `memcpy`) with LH macros; move to
  LH kernels. (The operator question is decided: see "Operators are not bricks;
  meanings are" in AGENTS.md.)
- [ ] Docs: `docs/README.md` index is half table, half trailing lines;
  `logging-follow-up.md` and `docs/reports/` are working notes mixed with docs.
- [ ] Versions: CHANGELOG has 4.0.0 (2026-10-24) and no 3.0.0 — reconcile
  with the branch workflow.

# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Fixed

- Windows build (MinGW): `authum_logger_ensure_dir` used the POSIX two-argument
  `mkdir`; the symbol test looked for `libc.so.6`, which exists only on Linux.

### Changed

- `CMakePresets.json` with `linux32` and `windows-mingw32` (configure, build,
  test). Configure fails at once, with a hint, when the compiler cannot link
  with `-m32`, instead of at the first link.

- **Module ABI 0.4.0**: `authum_engine_t` gains `query_client_cvar`. Modules
  built for 0.3 must be rebuilt (the loader refuses them).

- `<authum/json.h>` and `<authum/sql.h>` run on lh (`lh/json`, `lh/sql`) linked
  into `authum_client`; vendored cJSON (unused) and SQLite (`lib/cjson`,
  `lib/sqlite`) are gone, SQLite 3.46.1 now comes with lh. The module API is the
  same, and no lh type is in it; lh's symbols stay local to each module
  (`--exclude-libs,liblh.a`). Needs lh `0.4.0` with JSON/SQL; `-DAUTHUM_LH_DIR`
  builds against another lh tree.
- JSON fields come from a strict RFC 8259 reader: a key inside a string value
  is no longer found as a field, every escape is decoded (`\uXXXX` included),
  and a number must be an integer to read as `i32` (`1.5` was read as `1`).
- SQL on MySQL: `?` inside quotes, backquoted names and comments is no longer
  replaced; text of any length (embedded NULs included) instead of 255 bytes
  and a 2048-byte query; column indices are checked; server errors are kept for
  every step. The connection is utf8mb4.
- Host code follows lh's getter naming: `lh_os_module_get_path_as_const` is
  `lh_os_module_get_path`.

### Added

- More host events from Metamod hooks: `authum.client.command` (`ClientCommand`,
  a handler can block it), `authum.client.cvar_value` (`CvarValue2`, the answer
  to `QueryClientCvarValue2`), and `<authum/event/server.h>` with
  `authum.server.activate` / `deactivate` / `frame` (`ServerActivate` post,
  `ServerDeactivate`, `StartFrame`). The plugin now registers
  `GetEntityAPI2_Post` and `GetNewDLLFunctions`.
- `<authum/float.h>` — `authum_float_t` (mirrors `lh_float_t`); event payloads
  use it for `game_time` / `frame_time`.
- `authum_host_query_cvar` / `authum_engine_query_cvar` — ask a client for a
  cvar by entity index; the answer comes as `authum.client.cvar_value` with the
  same `request_id`.
- `authum_sql_stmt_bind_i64` / `authum_sql_stmt_column_i64` — 64-bit integers
  (SteamID64, row ids) in `<authum/sql.h>`.
- `<authum/crypto/ed25519.h>` — Ed25519 (RFC 8032) key pair / sign / check +
  `authum_crypto_wipe`, built into `authum_client` from vendored Monocypher 4.0.2
  (`lib/monocypher`). Client-side code, not a host facade: no ABI bump, modules
  get it without a newer `authum_mm`.
- `<authum/symbol.h>` — `authum_symbol_find` / `authum_symbol_find_engine`:
  address of a function or global in an already-loaded library, `dlsym` first,
  then the on-disk ELF `.symtab` (engine build 10211 keeps its internals local).
  Moved out of qproto so every module that detours the engine shares one copy.
- `<authum/detour.h>` — `authum_detour_t` + `attach` / `detach` / `suspend` /
  `resume`: 5-byte `jmp rel32` detour on 32-bit x86 for engine-thread hooks
  (was `qproto_hook_patch_jmp` / `restore_bytes`). Client-side, no ABI bump.
- `<authum/net.h>` — this machine's IPv4 addresses from its interfaces:
  `authum_net_ipv4_external` (public) and `authum_net_ipv4_internal`
  (LAN / NAT side), plus `is_external` / `is_loopback` / `format`. For HLDS
  bound to every interface, where the engine only knows `0.0.0.0`.
  Linux only; elsewhere the lists are empty. Client-side, no ABI bump.

## [2.0.0] - 2026-10-21

### Added

- `<authum/json.h>` — flat JSON object getters (`get_cstr` / `get_i32`) for API
  payloads; no full DOM
- `<authum/sql.h>` — module SQL brick (vendored SQLite amalgamation in
  `lib/sqlite`, `authum_sql_open_sqlite` / prepare / bind / step). Modules
  never include `sqlite3.h`. MySQL driver enum reserved.
- `authum_logger_file_split_*` + `authum_logger_open_module_file` /
  `build_module_path` / `ensure_dir` / `file_split_parse` — private log as one
  append file (`full`) or per-start under `logs/` (`session`)
- **Module ABI 0.3.0**: `authum_host_t` gains `sandbox`, `events` and
  `commands` (0.2 modules must be rebuilt)
- `<authum/commands.h>` — module console / RCON commands routed through the one
  engine command: `authum <module> <name> [args]`; built-ins `authum`,
  `authum help`, `authum version`, `authum <module>`. Modules never call
  `pfnAddServerCommand` (no clashes with AMX Mod X, no dangling commands after
  unload) ([docs/commands.md](docs/commands.md))
- Console module control (Metamod-like): `authum status` / `info` /
  `load` / `unload` / `reload` — turn a module off/on without restarting the
  server; subscriptions and module commands drop on unload and return on load
- `<authum/sandbox.h>` — per-module memory sandbox on lh's ownership tree
  (`lh_memory_tree`), freed by the host after `stop`; nested sandboxes,
  destructors ([docs/sandbox.md](docs/sandbox.md))
- `<authum/array.h>` — growable array stored in a sandbox
- `<authum/events.h>` — event bus between modules: forwards + queries by name,
  owner-tagged subscriptions dropped at unload ([docs/events.md](docs/events.md))
- `<authum/event/client.h>` — host publishes `authum.client.connect` (can
  refuse), `put_in_server`, `info_changed`, `disconnect` from Metamod dllapi hooks
- `authum_engine_kick` / `authum_host_kick` — sanitized `kick #userid "reason"`
- `authum_i64_t` / `authum_u64_t`
- `tests/` (CTest): client helpers, sandbox, event bus — 32-bit like HLDS
- `examples/hello` subscribes to `authum.client.connect` and uses a sandbox array

### Changed

- No `static inline` in public headers: all 64 helpers (host, env, path, cstr,
  engine, crypto, logger, version) now live in `src/client/*.c` inside
  `authum_client`; `authum_mm` links the same library
- No `static` functions in Authum sources — prefixed, linkable bricks
- C++-only flags (`-fno-rtti`, `-fno-exceptions`) no longer passed to C files
- `authum` server command output goes to the engine console (`pfnServerPrint`),
  so RCON sees it (was log-only)

### Fixed

- `authum_cstr_parse_i32` rejected `-2147483648` (INT_MIN)

### Added (ABI 0.2 groundwork)

- **Module ABI 0.2.0** (lh-free clients): `authum/client.h`, `authum_module_api`,
  host-filled `authum_host_t`; example [`examples/hello`](examples/hello/) links
  **no** liblh
- `authum_host_t.env` (`<authum/env.h>`) — `authum_path_t *` roots: game /
  models / sprites / addons / authum / modules / module (native volume;
  engine game-dir fallback)
- `<authum/path.h>` — `authum_path_t` (fixed capacity), join / parent / as_cstr
- `authum_client` static lib + `<authum/cfg.h>` — flat `key = value`
  (`get_*` / `apply_*` / `apply_items`); modules link the lib, not Authum sources
- `<authum/cstr.h>` — copy / `authum_cstr_eq(..., ignore_case)` / parse bool·i32·u32
- `authum_host_t.module_dir` — deprecated C string of `env->module`
- Docs: [docs/cfg.md](docs/cfg.md) (defaults + `apply_items`, custom keys outside
  the table), [docs/path.md](docs/path.md) (env roots / join)
- Architecture doc: host facade (B) + optional private deps

### Changed

- `authum_module_info` uses Authum `authum_version_t` / `date` string (not lh types)
- Child load prefers `authum_module_api`; legacy `authum_module_ops` still accepted

### Added

- `docs/module-style.md` — required naming / enum / Authum-type rules for child modules
- `<authum/logger.h>` — standard module logger (PSR-3 levels like lh; sinks host/console/file)
- Foundation types for modules (lh-shaped, no lh link): `authum_byte_t`, `authum_usize_t`,
  `authum_char_t`, `authum_ptr`, `authum_null`, `authum_bool_false` / `authum_bool_true`
- `authum/crypto/` — lh-shaped Rijndael + SHA facade for modules (`host->crypto` ops; not every lh symbol)
- `authum/engine.h` — engfuncs subset for modules (`host->engine`: game dir / print / command; no HLSDK)

### Removed

- In-tree qproto packaging (`src/modules/qproto`); qproto stays a separate project

## [1.0.0] - 2026-10-19

### Added

- Connect gate scaffolding (`authum_gate_init` / `deinit`) on the host
- Module list file `modules.list` (Authum dialect via `lh_str_line_reader`; not INI)
- Module load log: each path **OK** / **FAIL** (server log + console + `authum.log`)
- Server command `authum` — list loaded modules without `meta list`
- Host log: `lh_runtime_logger` → `authum.log`; children may use `authum_host_log`
- Host exports `authum_engine_funcs` / `authum_globals` for child modules
- `AUTHUM_LH_LINK_MODE`: `needed` (default) or `whole` (static lh only)
- Docs: writing a module, logging policy, gate, loader
- Linux CI on `master` (i386 Release) and release workflow that attaches `authum_mm.so` to tag `v*`
- Ready binary: [Releases](https://github.com/darwin128k/rapad-authum/releases) (`authum_mm.so`)

### Changed

- Authum builds **only** `authum_mm`; child modules (e.g. qproto) are separate drop-ins under `modules/`
- `lib/lh` submodule → https://github.com/darwin128K/lh.git branch `0.4.0`
- Branch `0.1.0` freezes the loader-only line; `master` is mainline after this release

## [0.1.0] - 2026-10-19

### Added

- Metamod-P host plugin `authum_mm`
- Module loader (`modules.list` or `modules/` scan) on lh
- Apache-2.0; `lib/lh` pinned to branch `0.4.0`

[Unreleased]: https://github.com/darwin128k/rapad-authum/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/darwin128k/rapad-authum/releases/tag/v1.0.0
[0.1.0]: https://github.com/darwin128k/rapad-authum/tree/0.1.0

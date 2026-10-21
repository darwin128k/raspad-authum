# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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

# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- `authum_cstr_parse_float`: a thin facade over the lh number parser, for the
  modules that may not link lh. The whole text must be the number, and a value
  too big for `authum_float_t` is refused.
- `authum_cstr_format` and `authum_cstr_format_v`: a thin Authum facade over
  the lh string formatter, for the modules that may not link lh. It cuts a
  result that does not fit, like the C runtime does, so a log line keeps the
  documented "the rest is cut" behaviour.

### Changed

- The module identity field `logtag` is now `tag`: it is the mark the module is
  shown with in a log record, not a logging thing of its own. A module built
  against the old name does not compile — the module ABI moves to **3.0.0**,
  and no 2.x module loads. The three words are separated now: `owner` is who
  owns a subscription, a journal or a decision; `tag` is the mark a source is
  shown with; `name` is what it is called.
- Standard file logs use the HLDS `L MM/DD/YYYY - HH:MM:SS:` timestamp prefix,
  followed by the level and text. Shared-host copies retain their module/journal
  tag; a module's own file does not repeat its name.

- Module commands run on the LH console (`lh/console`): the registry no longer cuts
  usage and help text, and `authum_command_print` / `authum_command_printf` print a line
  of any length. `authum_command_line_max`, `authum_command_usage_max` and
  `authum_command_help_max` are gone; `authum_command_word_max` stays.
- HTTP serves on Windows as on Linux: one transport on LH sockets, file handles and
  formatting; a source path may be absolute in either OS's form. The server starts and
  stops Winsock itself.
- The HTTP worker is an LH loop that sleeps in `poll` on its sockets; it no longer
  takes one of the module's 64 thread slots. `authum_http_host_init` takes no thread
  facade.
- A module's threads are an LH thread group (`lh/os/thread/group`).
- Log files are written by the LH log queue and daily files (`lh/os/logger`): the
  writer sleeps until a record instead of polling every 10 ms, records carry a
  channel id instead of a copy of the directory path, open files are no longer
  capped at 32, and Windows and Linux share one path with no `#if`.
- `authum_command_def_t` has level-1 accessors (`authum_command_def_get_name`, ...).
- `authum_net_address_host` is `authum_net_address_copy_host`, declared in its own
  `<authum/net/address.h>` (still brought in by `<authum/client.h>`).
- Module API sources mirror their headers (`src/authum/api/command/def.c` for
  `<authum/command/def.h>`, ...); `tools/check-mirror.py` checks it.

### Added

- Module ABI 2.0.0 for Authum 0.4.0, with no backward compatibility: the host tables
  and the event payloads lose their `size` member, `<authum/abi.h>` (`AUTHUM_ABI_FITS` /
  `AUTHUM_ABI_HAS`) is gone, and no 1.x module loads. From here the ABI is frozen:
  nothing that exists changes, new API is only appended, and a module needs a host of
  the same major and the same or a later minor (`authum_version_is_abi_compatible`).
  Host tables are built with designated initializers.
- Indexed event dispatch through the LH module event bus (`lh/os/module/event`) and
  numeric event operations. A module handler is the bus callback itself, with its user
  value as the context: no per-subscription allocation. Game hooks and messages cache
  event IDs.
- Automatic-channel `authum_host_log_*` conveniences and ABI 1.4
  `authum_logger_create` for named sandbox journals with host-owned standard
  or raw formatting. Existing logger handles and older host tables remain supported.
- Module-local logging preferences from `configs/<module>.cfg`, inherited
  global defaults, explicit administrator overrides and startup-only
  `AUTHUM_LOG_LEVEL` source filtering.
- Independent private-file, host-copy and console settings, and per-channel
  retention. Worker-side cleanup runs at registration/reload and every 24
  hours, including idle channels, without deleting open files or invalid dates.
- Managed thread service (ABI 1.1): `authum_host_get_threads(host)` starts, stops
  and joins long-lived workers per module owner. The host wakes and joins the
  workers of an unloading module before calling its stop callback.
- Shared HTTP service and monotonic clock (ABI 1.2): owner-bound static-file
  servers with source, extension and size policy. Modules no longer implement
  sockets, request parsing or worker lifecycle.
- Placeholder service (ABI 1.3): the host resolves `{game}`, `{module}`,
  `{data}`, `{config}`, `{lang}` and the per-module registrations, so a module
  never parses a path template itself.

### Changed

- Public API names say what they return (99 functions): values are `get_` / `is_` /
  `has_` (`authum_host_get_engine`, `authum_array_get_size`, `authum_command_get_argv`,
  `authum_env_get_game_path`, `authum_events_get_id`, `authum_clock_get_monotonic_ms`,
  `authum_logger_buffer_is_active`, ...), results written into the caller's buffer are
  `copy_` (`authum_host_client_copy_name`, `_copy_auth_id`, `_copy_info`,
  `authum_engine_copy_map_name`, `authum_cfg_copy_cstr`, `authum_json_object_copy_cstr`),
  scalars through a pointer with a success flag are `try_get_`
  (`authum_host_entity_try_get_float`, `authum_command_try_get_arg_i32`,
  `authum_json_object_try_get_i32`; `authum_json_object_get_i32` takes the fallback),
  path builders are `join_` (`authum_host_join_config_path`), and
  `authum_cfg_open_file`, `authum_cstr_is_equal`, `authum_char_to_lower`,
  `authum_crypto_ed25519_is_valid` / `_create_key_pair`, `authum_path_get_cstr` /
  `_has_trailing_sep` / `_assign_parent`, `authum_net_ipv4_collect_external` /
  `_collect_internal`. `.clang-format` matches LH's.
- Thread, task and logging workers use LH OS primitives behind the existing module API.

- HTTP request parsing now uses LH's portable parser; removed PicoHTTPParser.
  The public module API remains unchanged.

- `log_level=off` now disables every destination and the module's journals;
  use `log_file=off` to disable only the private file. The default retention
  and shipped configuration are 10 days; explicitly configured values are preserved.
- This changes module logging only. The native HLDS asynchronous file writer
  remains an isolated Linux experiment and is not enabled by this release.

## [4.0.0] - 2026-10-24

### Changed

- **Metamod is gone — Authum is the game library and nothing else.** The
  Metamod-P plugin image (`authum_mm.so`, `src/authum/plugin/`) is removed, the
  core no longer speaks Metamod (no `plugin_info_t`, no `meta_globals_t`, no
  Metamod console mirror — the host mirrors console-level lines to the engine
  console itself), and the HLSDK headers are vendored on their own
  (`lib/hlsdk`) instead of arriving with Metamod-P. One build, one image:
  `authum.so`.
- **The game library is loaded through lh's OS layer** (`dlopen` / `LoadLibrary`),
  so the same image builds on Linux and Windows.
- **`src/authum/` mirrors `include/authum/`, sources named for what they are.**
  `src/standalone/` → `src/authum/gamedll/` (what the engine sees as the game
  DLL: entry points, the real game behind it, hooks, class trampolines), the
  core → `src/authum/host/`, `src/client/` → `src/authum/api/` (the module API
  behind `include/authum/`; the CMake target is still `authum_client`, and
  `client/CMakeLists.txt` stays where modules include it).
  `docs/standalone.md` → `docs/gamedll.md`.
- **`include/authum/` is the module API only.** The host's headers moved next
  to their code as `<authum/host/…>`: `gate.h`, `loader.h`, `log.h`, `module.h`,
  `console.h`, `error/code.h` → `errors.h`, and the five `*/host.h` →
  `events.h`, `commands.h`, `logs.h`, `sandbox.h`, `engine.h`. No public header
  includes lh any more.
- **`AUTHUM_EXTERN_C_BEGIN` / `AUTHUM_EXTERN_C_END`** (`<authum/extern.h>`)
  replace the `#ifdef __cplusplus` / `extern "C" {` pair in every header.
- **`authum_unused(x)`** (`<authum/unused.h>`, in `<authum/client.h>`) replaces
  `(void)x;` for parameters a handler does not need.
- **Authum types on the API surface:** `authum_cptr` (`const void *`) for
  `authum_array_push`, `authum_ptr` for `authum_crypto_wipe` and the SQL
  handles, `authum_bool_t` for `authum_host_log_line`'s `is_error`. Examples
  drop casts that converted nothing; `docs/module-style.md` gains the macro
  case rule and the `printf` format of each Authum type.
- **No platform code in `authum_client`.** Directories, page protection
  (detours), symbols of loaded images (exported and ELF-local) and interface
  addresses now come from lh's os layer (`lh_os_fs_dir_create`,
  `lh_os_memory_protect`, `lh_os_shared_bind_name` / `_get_local_sym`,
  `lh_os_net_interface_each_ip4`); `src/authum/api/os/` is gone, and so are
  the per-platform filters and the `iphlpapi` / `dl` links (lh brings them).
  Same behaviour — a library loaded by full path is still found by its file
  name.
- **`authum_cstr_parse_i32` / `_u32` count digits with lh**
  (`lh_str_ptr_parse_ullong_digits`), same rules as before: spaces around,
  a `+` (and `-` for i32), zero padding allowed. `array`, `cfg` and the
  `authum_path_t` text stay Authum's: the array lives in the module's sandbox,
  the cfg reader skips bad lines where lh's INI parser stops, and
  `authum_path_t` is part of the host ABI.
- **Ed25519 comes from lh.** `<authum/crypto/ed25519.h>` now wraps
  `lh/crypto/ed25519.h` the way Rijndael and SHA wrap theirs; Monocypher moved
  into lh (`lib/lh/lib/monocypher`) and `lib/monocypher` with the
  `authum_monocypher` target is gone. Same API, same bytes; lh's symbols —
  Monocypher's now among them — stay local to each module (`--exclude-libs`).

### Added

- **Entity-class trampolines** (`src/authum/gamedll/game_classes.cpp`, generated by
  `tools/gen-game-classes.py`): the engine resolves an entity's spawn function
  by its class name *in the game module* — the library it loaded, which is us —
  so every class name the real game exports is exported by Authum too and
  forwards to the game. Without them the engine silently skips the entities
  and the game dies on an empty entity hash (the failure Metamod-R hit on
  ReHLDS).

- **Standalone build: Authum as the game library** (`authum.so`, Linux i386),
  no Metamod in between. The engine loads it where the game library used to
  be (`liblist.gam`: `gamedll_linux "addons/authum/authum.so"`), Authum loads
  the real game behind it and overlays its event hooks on the game's
  callbacks. The layout for the admin is the Metamod one: `addons/authum/`
  with `configs/`, `logs/` and `modules/`. The real game library is named in
  `addons/authum/gamedll.txt`; the build links it in
  (`-DAUTHUM_GAME_DLL_NAME=cs.so`) because the engine resolves every map
  entity's spawn function by its class name in the game module. See
  [docs/standalone.md](docs/standalone.md).

### Fixed

- **`authum.message.*` and `authum.server.log` work again.** Without Metamod
  nothing hooked the game's engine calls, so these events never fired (a
  `DeathMsg` handler stayed silent). The game now gets the engine with
  Authum's hooks over `RegUserMsg` (message names), `MessageBegin` …
  `MessageEnd` and `AlertMessage` (`src/authum/gamedll/engine.cpp`). A held
  message reaches the engine once — the 3.0.0 Metamod plugin re-sent it
  through Metamod, so other plugins saw it twice.
- Lines Authum mirrors to the server console end with a newline (they ran
  into the next output).
- The host image's `authum_module_info` lives in the core now: the standalone
  build did not define it, and an undefined symbol keeps the image from
  loading at all.
- The loader rejects the host image by name (`authum_mm`, `authum`), so the
  standalone image cannot be opened as its own child module.
- Windows build (MinGW): `authum_logger_ensure_dir` used the POSIX two-argument
  `mkdir`; the symbol test looked for `libc.so.6`, which exists only on Linux.
- `authum_net_ipv4_collect_internal` / `_external` list the interfaces on Windows too
  (they were always empty there).
- `authum_engine_query_cvar` had a raw NUL byte in a character literal, so git
  took `src/client/engine.c` for a binary file.

### Changed

- A module must export `authum_module_api`; the loader no longer takes a
  child that only exports lh's `authum_module_ops` (that table is now just
  how `authum_mm` itself binds to lh). `AUTHUM_TRUE` / `AUTHUM_FALSE` are
  gone: use `authum_bool_true` / `authum_bool_false`.
- **Every module is a sandbox**: `addons/authum/modules/<name>/` holds
  `<name>.so`, `configs/` (the admin's), `lang/` (translations), `data/`
  (what the module writes) and `logs/`. Install or remove a module by its
  folder; an update replaces the image and `lang/`, never `configs/`,
  `data/` or `logs/`. Authum keeps only its own: `configs/authum.cfg`,
  `configs/modules.list`, `logs/`. `authum_env_t` starts with `size`;
  `module` is the module's folder, with `module_config`, `module_lang`,
  `module_data` (created at load); `authum_host_t.module_dir` is gone.
  `authum_host_join_config_path` → `modules/<name>/configs/<file>`,
  `authum_host_join_data_path` → `modules/<name>/data/<file>`. A
  `modules.list` entry `qproto` loads `modules/qproto/qproto.so`; without
  the list every module folder is loaded. The release zip ships `configs/`.
- Everything authum_client asks of the OS (directories, page protection,
  library symbols, interface addresses) sits behind `src/client/os/os.h`, with
  one implementation in `os/posix/` and one in `os/win/`; CMake builds the
  target's. The rest of `src/client` has no `#if _WIN32`. No API change.
- `CMakePresets.json` with `linux32` and `windows-mingw32` (configure, build,
  test). Configure fails at once, with a hint, when the compiler cannot link
  with `-m32`, instead of at the first link.

- **Logging is decided by Authum, not by each module.** Every module writes
  `modules/<module>/logs/YYYY-MM-DD.log` (a new file each day);
  Authum's own lines and every module's warnings and errors (tagged
  `[module]`) go to `addons/authum/logs/`. Levels live in one new file,
  `addons/authum/configs/authum.cfg`: `log_level`, `log_console` (what also reaches
  the server console / HLDS log through Metamod) and `<module>.log_level`.
  `addons/authum/authum.log` is gone. A module just calls
  `authum_logger_init(&log, host)` and `authum_logger_info` / … ; lines below
  the level are dropped before formatting. Removed: sinks
  (`authum_logger_sink_*`, `logging_mode`-style masks), private files
  (`authum_logger_open_file`, `open_module_file`, `build_module_path`,
  `file_split_*`), `set_level_flags` / `set_sinks` / `ensure_dir`,
  `authum_host_get_log` / `set_log` and the host's `authum_host_log` export.
  `authum_host_t.log` became `authum_host_t.logger` (a channel). New:
  `authum_logger_is_enabled`, `authum_logger_level_parse` / `_name`,
  `authum_path_ensure_dir`. The release zip ships a commented `authum.cfg`.
  - Journals: `authum_logger_open(&log, host, "bans")` gives a module an extra
    log, `modules/<module>/logs/bans/YYYY-MM-DD.log`, with its own level
    (`<module>.bans.log_level`).
  - `log_keep_days`: daily files older than that are deleted (shipped cfg: 30).
  - Lines are buffered and flushed every server frame; errors and worse go to
    disk at once.
  - `authum.cfg` is read again on every map change.
- 32-bit Linux builds use `_FILE_OFFSET_BITS=64` (Authum, lh, authum_client):
  `readdir()` failed with `EOVERFLOW` on file systems with 64-bit inode
  numbers (XFS, btrfs, network mounts), so the `modules/` scan found nothing.
- **Module ABI 1.0.0** (modules built for 0.x must be rebuilt):
  - Every table the host hands a module (`authum_host_t`, `authum_engine_t`,
    `authum_crypto_t`, the events / sandbox / commands ops) starts with
    `authum_usize_t size`; functions are only appended, and the client
    wrappers call one only when the host's table reaches it
    (`AUTHUM_ABI_HAS`, `<authum/abi.h>`). Adding a function is now a minor
    bump: a 1.x module loads on any 1.x host (`authum_version_is_abi_compatible`).
  - Keep the host with `authum_host_copy(&g_host, host)`, not `g_host = *host`.
  - `authum_engine_t` covers what modules took from raw `engine_funcs`:
    `max_clients`, `game_time`, `get_map_name`, `cvar_get` / `cvar_set`,
    `client_is_connected`, `client_userid`, `client_index_by_userid`,
    `client_get_name` / `_auth_id` / `_address` / `_info`, `client_print`
    (console / center / chat, 0 = everyone), `client_command`,
    `query_client_cvar` — each with `authum_engine_*` and `authum_host_*`
    wrappers. Players are addressed by entity index, as in `authum.client.*`.

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

- Engine facade, more of what modules took from raw `engine_funcs`:
  user messages (`authum_engine_message_get_id` / `_begin` / `_write_byte` …
  `_write_string` / `_end`, destinations and argument kinds in
  `<authum/engine/message.h>`), `authum_engine_client_chat` (colored chat
  through the game's `SayText`), `cvar_register` (a module's own server cvar,
  `authum_engine_cvar_*` flags) with `cvar_get_i32` / `cvar_get_float`,
  `client_get_stats` (ping, loss) and `client_set_info` — each with an
  `authum_host_*` shortcut.
- Game user messages as events: subscribe to `authum.message.<Name>`
  (`authum.message.DeathMsg`, `SayText`, …) to read the whole message
  (`<authum/event/message.h>`, `authum_event_message_get_*`), change its
  arguments (`_set_*`) or `block` it. Only messages someone listens to are
  held back; a held message is sent again after the handlers, so other
  Metamod plugins hooking messages see it twice. The plugin now hooks
  `GetEngineFunctions` (MessageBegin … MessageEnd, AlertMessage).
- `authum.server.log`: every line of the HLDS log (`log on`) — the kill /
  round / team lines stats are built from (`authum_event_server_log_t`).
- `authum.client.kill` (`ClientKill`; `allow = false` refuses it),
  `authum.server.precache` (worldspawn's `Spawn`) with
  `authum_engine_precache_model` / `_sound` / `_generic` — 0 outside the
  precache window instead of the engine's Host_Error — and
  `authum.server.shutdown` (`GameShutdown`).
- Entities without HLSDK (`<authum/engine/entity.h>`): `entity_max`,
  `entity_is_valid`, `entity_get_classname`, `entity_find_by_classname`,
  `entity_get_float` / `_set_float` (health, armor, frags, gravity, max speed,
  take damage, …), `entity_get_i32` / `_set_i32` (flags, dead, buttons,
  weapons, render mode, …), `entity_get_vector` / `_set_vector` (origin —
  set through SET_ORIGIN — velocity, angles, view angle, mins / maxs — set
  through SET_SIZE — …), with `authum_entity_flag_*` / `authum_entity_dead_*`
  values; `authum_engine_client_is_alive` / `_is_bot`. Field numbers never
  change; game private data (a CS player's money or team) stays behind the
  `engine_funcs` escape hatch.
- `<authum/timer.h>`: `authum_timer_add(host, delay, interval, fn, user)` /
  `_remove` / `_clear` / `_count` — module timers on server frames, counting
  across map changes, dropped with the module.
- `examples/`: module templates, one topic each, all loaded and exercised by
  the new CTest `authum_examples` (real event bus, commands, sandbox and logs,
  a fake engine): `skeleton` (the empty template), `hello` (updated to the
  new logger), `welcome` (player / map events, engine facade), `chatfilter`
  (block a client command), `cvarguard` (client cvar query, kick),
  `settings` (module cfg with defaults, a checked key, reload), `stats`
  (SQLite + a journal), `scores` + `scoreboard` (two modules over the event
  bus with a types-only header), `deathnotice` (`DeathMsg`, colored chat,
  module cvars, refusing `kill`), `announcer` (timers, precache, HLDS log
  lines, entity fields). Built with `-Wall -Wextra`, warning-free.
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
  `authum_net_ipv4_collect_external` (public) and `authum_net_ipv4_collect_internal`
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
- `<authum/cstr.h>` — copy / `authum_cstr_is_equal(..., ignore_case)` / parse bool·i32·u32
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
- Ready binary: [Releases](https://github.com/darwin128k/raspad-authum/releases) (`authum_mm.so`)

### Changed

- Authum builds **only** `authum_mm`; child modules (e.g. qproto) are separate drop-ins under `modules/`
- `lib/lh` submodule → https://github.com/darwin128K/lh.git branch `0.4.0`
- Branch `0.1.0` freezes the loader-only line; `master` is mainline after this release

## [0.1.0] - 2026-10-19

### Added

- Metamod-P host plugin `authum_mm`
- Module loader (`modules.list` or `modules/` scan) on lh
- Apache-2.0; `lib/lh` pinned to branch `0.4.0`

[Unreleased]: https://github.com/darwin128k/raspad-authum/compare/v4.0.0...HEAD
[4.0.0]: https://github.com/darwin128k/raspad-authum/releases/tag/v4.0.0
[3.0.0]: https://github.com/darwin128k/raspad-authum/releases/tag/v3.0.0
[2.0.1]: https://github.com/darwin128k/raspad-authum/releases/tag/v2.0.1
[2.0.0]: https://github.com/darwin128k/raspad-authum/releases/tag/v2.0.0
[1.0.0]: https://github.com/darwin128k/raspad-authum/releases/tag/v1.0.0
[0.1.0]: https://github.com/darwin128k/raspad-authum/tree/0.1.0

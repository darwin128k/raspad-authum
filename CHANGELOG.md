# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- **Module ABI 0.2.0** (lh-free clients): `authum/client.h`, `authum_module_api`,
  host-filled `authum_host_t`; example [`examples/hello`](examples/hello/) links
  **no** liblh
- `authum_host_t.module_dir` — module image directory for cfg/data
- Architecture doc: host facade (B) + optional private deps

### Changed

- `authum_module_info` uses Authum `authum_version_t` / `date` string (not lh types)
- Child load prefers `authum_module_api`; legacy `authum_module_ops` still accepted

### Added

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

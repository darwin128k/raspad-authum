# Changelog

## 1.0.0

- Connect gate scaffolding (`authum_gate_init` / `deinit`) on the host
- Module list file `modules.list` (Authum dialect via `lh_str_line_reader`; not INI)
- Module load log: each path **OK** / **FAIL** (server log + console + `authum.log`)
- Server command `authum` — list loaded modules without `meta list`
- Host log: `lh_runtime_logger` → `authum.log`; children may use `authum_host_log`
- Host exports `authum_engine_funcs` / `authum_globals` for child modules
- `AUTHUM_LH_LINK_MODE`: `needed` (default) or `whole` (static lh only)
- Docs: writing a module, logging policy, gate, loader
- `lib/lh` submodule → https://github.com/darwin128K/lh.git branch `0.4.0`
- Branch `0.1.0` freezes the loader-only line

## 0.1.0

- Metamod-P host plugin `authum_mm`
- Module loader (`modules.list` or `modules/` scan) on lh
- Apache-2.0; `lib/lh` pinned to branch `0.4.0`

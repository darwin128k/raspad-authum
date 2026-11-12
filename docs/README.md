# Authum docs

Build and drop-in layout: the repository [README](../README.md).

## Writing a module

| Doc | What it covers |
|-----|----------------|
| [writing-a-module.md](writing-a-module.md) | How to write your own module: exports, layout, engine functions, checklist |
| [module-style.md](module-style.md) | Naming, enums and Authum types: the style child modules follow |
| [api/](api/) (generated) | The module ABI reference from `include/authum`: `cmake --build <preset> --target authum_api_docs` → `api/html/index.html` |
| [../examples/](../examples/) | Module templates, one topic each, all run by `tests/examples.cpp` |
| [cfg.md](cfg.md) | `key = value` config files: defaults, `apply_items`, custom keys |
| [path.md](path.md) | `authum_path_t`, the host's env roots, joining config and log leaves |
| [placeholders.md](placeholders.md) | Path placeholders: standard names and per-module registrations |
| [memory.md](memory.md) | The Authum heap, the LH allocator, failure and ownership contracts |
| [region.md](region.md) | The module's memory region (freed at unload), nested regions, `authum_array_t` |
| [events.md](events.md) | The event bus: forwards and queries between modules, `authum.client.*` player events |
| [hooks.md](hooks.md) | The hook chain: who decides, in what order, within what budget |
| [commands.md](commands.md) | `authum <module> <cmd>` console / RCON commands, and why modules never register engine commands |
| [shared-services.md](shared-services.md) | Worker logs, client references, SQL error history, address parsing |
| [sql.md](sql.md) | SQL handles, worker queries, callbacks, connection recovery |
| [threads.md](threads.md) | Managed long-lived workers |
| [http.md](http.md) | The shared HTTP service and monotonic clocks |
| [logging.md](logging.md) | Where logs go and the host / module logging policy |

## How the host works

| Doc | What it covers |
|-----|----------------|
| [architecture.md](architecture.md) | The host facade over LH, and a module's optional private dependencies |
| [strategy.md](strategy.md) | How the core, the extensions and the modules fit together |
| [gamedll.md](gamedll.md) | Authum as the game library: the engine's contract, build options |
| [modules.md](modules.md) | The loader: `modules.list`, the directory scan, host lifetime |
| [gate.md](gate.md) | The connect gate and external auth modules |
| [scripting.md](scripting.md) | The script layer: how a script reaches the game |

## Plans

| Doc | What it covers |
|-----|----------------|
| [roadmap.md](roadmap.md) | What comes next for Authum |
| [cleanup.md](cleanup.md) | The debt that is left; deleted when it is empty |
| [../lib/lh/docs/roadmap.md](../lib/lh/docs/roadmap.md) | What comes next for LH |

## Reports

Dated measurements and investigations. They record what was seen at the time and
are not updated afterwards.

| Report | What it records |
|--------|-----------------|
| [reports/hlds-log-buffering/](reports/hlds-log-buffering/) | The engine's own log is written late: the upstream report and the follow-up investigation |
| [reports/lh-pointer-assertions-2026-10-31.md](reports/lh-pointer-assertions-2026-10-31.md) | Duplicate pointer assertions in LH: the inventory |
| [reports/lh-release-benchmark-2026-10-31.md](reports/lh-release-benchmark-2026-10-31.md) | LH Release benchmarks |

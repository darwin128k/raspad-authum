# Authum docs

| Doc | What it covers |
|-----|----------------|
| [api/](api/) (generated) | Module ABI reference from `include/authum` ? `cmake --build ? --target authum_api_docs` ? `api/html/index.html` (AMXX `/api` analogue) |
| [architecture.md](architecture.md) | Host facade (B) + optional private deps (?hole?) |
| [writing-a-module.md](writing-a-module.md) | How to write your own Authum module (exports, layout, engfuncs, checklist) |
| [module-style.md](module-style.md) | Naming / enums / Authum types ? required style for child modules |
| [cfg.md](cfg.md) | Flat `key=value` config: `authum_client`, defaults + `apply_items`, custom keys |
| [path.md](path.md) | `authum_path_t`, host env roots, joining cfg/log leaves |
| [sandbox.md](sandbox.md) | Per-module memory sandbox (freed at unload), nested sandboxes, `authum_array_t` |
| [events.md](events.md) | Event bus: forwards + queries between modules, `authum.client.*` player events |
| [commands.md](commands.md) | `authum <module> <cmd>` console / RCON commands ? why modules never register engine commands |
| [../examples/](../examples/) | Working `hello` module (`authum/host.h`) |
| [modules.md](modules.md) | Loader model, `modules.list`, directory scan, host lifetime |
| [logging.md](logging.md) | Where logs go today and the planned host/module logging policy |
| [gate.md](gate.md) | Connect gate / external auth modules |

Build and drop-in layout: see the repo [README](../README.md).

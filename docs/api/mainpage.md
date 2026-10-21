# Authum module API

Browsable reference for **child modules** (`authum_client` / `<authum/…>`).

Same role as [AMX Mod X Scripting API](https://amxmodx.org/api/): one place to
look up symbols. Source of truth is `include/authum/**/*.h` — this site is
generated, not hand-copied.

| Start here | Header |
|------------|--------|
| Module export | `<authum/module/api.h>`, `<authum/module/info.h>` |
| Host facade | `<authum/host.h>`, `<authum/client.h>` |
| Events / commands | `<authum/events.h>`, `<authum/commands.h>` |
| Bricks | `<authum/cfg.h>`, `<authum/sql.h>`, `<authum/json.h>`, `<authum/logger.h>`, … |

Guides (how to write a module): see `docs/` markdown next to this tree.
Local build: `cmake --build <build> --target authum_api_docs`, then open
`docs/api/html/index.html`.

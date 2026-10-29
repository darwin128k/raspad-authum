# Examples

Each folder is a complete Authum module on one topic. Copy the one closest to
what you need, rename it, and build from there. They use only
`<authum/client.h>` and link `authum_client` — exactly what your module does.

| Example | Start here when you want to… | Shows |
|---------|------------------------------|-------|
| [skeleton](skeleton/) | write any module | the empty template: info, start / stop, host copy, logger |
| [hello](hello/) | see the usual pieces together | event subscription, sandbox array, console command |
| [welcome](welcome/) | react to players and maps | `put_in_server` / `disconnect` / `server.activate`, engine facade: name, auth id, address, chat message, player list |
| [chatfilter](chatfilter/) | stop a player's command | `authum.client.command` with `block` |
| [cvarguard](cvarguard/) | check a client's settings | `authum_host_query_cvar` → `authum.client.cvar_value`, kick |
| [settings](settings/) | give your module a config file | defaults in a struct, `authum_cfg_apply_items`, a key with its own check, reload command |
| [stats](stats/) | store data | SQLite through `<authum/sql.h>`, a separate journal (`authum_logger_open`) |
| [scores](scores/) + [scoreboard](scoreboard/) | let two modules talk | a public types-only header, a forward and a query on the event bus |
| [deathnotice](deathnotice/) | read or change a game message | `authum.message.DeathMsg`, colored chat, the module's own server cvars, refusing `kill` |
| [announcer](announcer/) | do something every N seconds or on a log line | timers, precache, `authum.server.log`, entity fields (health, armor) |

Every example is loaded and exercised by `tests/examples.cpp` (CTest
`authum_examples`), so they keep working as the API moves.

## Build

They build with Authum (`AUTHUM_BUILD_EXAMPLES`, on by default):

```sh
cmake --preset linux32                  # or windows-mingw32
cmake --build --preset linux32 --target welcome
```

Output: `build-linux32/examples/<name>/<name>.so` (`.dll` on Windows).

In your own project, add Authum's `client/` directory and link the same way —
see the `authum_example` function in [CMakeLists.txt](CMakeLists.txt):

```cmake
add_subdirectory(lib/authum/client)
add_library(mymod MODULE mymod.c)
set_target_properties(mymod PROPERTIES PREFIX "")
target_link_libraries(mymod PRIVATE authum_client)
```

## Install

```text
addons/authum/
  configs/modules.list         ← one line per module: welcome
  modules/welcome/welcome.so
  modules/settings/configs/settings.cfg   (settings only, optional)
  modules/stats/data/stats.db             (written by stats)
```

Then `authum status` in the server console. Each module logs to
`addons/authum/modules/<name>/logs/`; levels are set in `addons/authum/configs/authum.cfg`.

More: [docs/writing-a-module.md](../docs/writing-a-module.md),
[docs/events.md](../docs/events.md), [docs/logging.md](../docs/logging.md).

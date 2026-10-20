# Logging

Modules use Authum’s **standard logger** (`<authum/logger.h>`), shaped like
lh’s levels. Want a custom setup? Write it from scratch, or reuse the same
**bricks** (host channel, engine console, private `FILE *`).

---

## 1. Standard logger (preferred)

```c
#include <authum/client.h>

static authum_logger_t g_log;

static authum_bool_t
mod_start(authum_host_t *host)
{
    authum_path_t path;

    authum_logger_init(&g_log, host,
                       authum_logger_sink_host | authum_logger_sink_file,
                       "mymod");
    authum_path_join(&path, authum_host_env_module(host), "mymod.log");
    authum_logger_open_file(&g_log, authum_path_as_cstr(&path));

    authum_logger_info(&g_log, "start");
    return authum_bool_true;
}

static void
mod_stop(void)
{
    authum_logger_info(&g_log, "stop");
    authum_logger_close(&g_log);
}
```

| Piece | Role |
|-------|------|
| `authum_logger_level_*` | PSR-3 severity (emergency … debug), like lh |
| `authum_logger_level_flags_*` | Which levels are emitted |
| `authum_logger_sink_*` | Where lines go (bitmask) |
| `authum_logger_info` / `_error` / … | Shortcuts |

### Sinks (`LoggingMode`-compatible bits)

| Bit | Symbol | Destination |
|-----|--------|-------------|
| 0 | `authum_logger_sink_none` | Drop |
| 1 | `authum_logger_sink_console` | `host->engine` → server print |
| 2 | `authum_logger_sink_file` | Private file from `authum_logger_open_file` |
| 4 | `authum_logger_sink_host` | `host->log` → `authum.log` |

Cfg integers such as qproto’s `LoggingMode` can be passed straight as the
sink mask.

---

## 2. Bricks (DIY)

| Brick | How |
|-------|-----|
| Host / `authum.log` | `authum_host_log_line(host, is_error, line)` or `host->log` |
| Console | `authum_engine_server_print(host->engine, text)` |
| Private file | `fopen` / `fprintf` yourself |

Do **not** open `authum.log` from a module. Do **not** assume
`lh_runtime_logger()` in a child `.so` is the host log (escape-hatch lh
has its own instance).

---

## 3. Host side (today)

| Channel | Where |
|---------|--------|
| Host | `addons/authum/authum.log` via lh + Metamod mirror |
| Modules | `authum_logger` → sinks above |

---

## 4. Future host policy (unchanged intent)

Configurable host scope (`off` / `host` / `full`) may later filter what
module `sink_host` lines enter `authum.log`. Module-private files stay
under the module’s control.

---

## Related

- [module-style.md](module-style.md)
- [writing-a-module.md](writing-a-module.md)
- [architecture.md](architecture.md)

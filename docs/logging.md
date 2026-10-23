# Logging

A module says **what happened and how bad it is**. Authum decides **where it
goes**, from one file, `addons/authum/configs/authum.cfg`. No module has its own
logging keys, file paths or console switches.

---

## 1. Where lines end up

```text
addons/authum/
  authum.cfg
  logs/
    authum/2026-10-22.log    ← Authum itself + every module's warnings and errors
    qproto/2026-10-22.log    ← one folder per module, one file per day
    qproto/bans/2026-10-22.log  ← a journal the module opened
    hello/2026-10-22.log
```

| Line | Goes to |
|------|---------|
| Module line at or above the module's `log_level` | `logs/<module>/YYYY-MM-DD.log` |
| Module **warning or worse** | also `logs/authum/…`, tagged `[module]` |
| Authum's own line (loader, gate, commands) | `logs/authum/…` |
| Any line at or above `log_console` | also the server console and the HLDS log (through Metamod, `[AUTHUM]`) |

A new file starts when the date changes; files older than `log_keep_days` are
deleted then. Lines are buffered and reach the disk every server frame;
errors and worse are written at once, so a crash does not lose them. A line
looks like:

```text
[15:30:02] [warning] [qproto] ban list not found, starting empty
```

(the `[qproto]` tag only appears in `logs/authum/`).

---

## 2. Configuration (`authum.cfg`)

```ini
log_level = info          # what reaches the files
log_console = warning     # what also reaches the console / HLDS log
log_keep_days = 30        # delete daily files older than this (0 = keep all)
qproto.log_level = debug  # one module louder (or `off`), by its `authum status` name
qproto.bans.log_level = info  # one journal of a module (default: the module's)
authum.log_level = info   # Authum's own channel
```

Levels, quietest to loudest (PSR-3, like lh): `emergency`, `alert`,
`critical`, `error`, `warning`, `notice`, `info`, `debug`; `off` turns a
channel off. Without `authum.cfg`: `log_level = info`, `log_console = warning`.

The file is read when the server starts and again on **every map change**, so
an edit applies from the next map without a restart.

---

## 3. In a module

```c
#include <authum/client.h>

authum_host_t g_host;
authum_logger_t g_log;

authum_bool_t
mod_start(authum_host_t *host)
{
    authum_host_copy(&g_host, host);
    authum_logger_init(&g_log, &g_host);
    authum_logger_info(&g_log, "start");
    return authum_bool_true;
}

void
mod_stop(void)
{
    authum_logger_info(&g_log, "stop");
    authum_logger_close(&g_log);
}
```

| Call | |
|------|--|
| `authum_logger_emergency` … `authum_logger_debug` | printf-style line at that level |
| `authum_logger_is_enabled(&log, level)` | skip expensive work for a level nobody records |
| `authum_logger_hex_dump(&log, label, data, len)` | debug-level hex dump |
| `authum_host_log_line(host, is_error, text)` | one preformatted line (error or info) |
| `authum_logger_open(&log, host, "bans")` | a journal: own folder and level, same calls |

### Journals

A task that wants its own file — bans, payments, an admin audit trail — opens
a journal. Same logger, two lines:

```c
authum_logger_t g_bans;

authum_logger_open(&g_bans, &g_host, "bans");          /* start */
authum_logger_info(&g_bans, "%s banned %s", admin, target);
authum_logger_close(&g_bans);                          /* stop */
```

It writes `logs/<module>/bans/YYYY-MM-DD.log` at `<module>.bans.log_level`
(default: the module's level); its warnings and errors reach `logs/authum/`
tagged `[module/bans]`. Opening the same name twice shares the journal. A
journal the module forgets to close is closed after `stop`.

Lines below the configured level are dropped **before formatting**, so
`authum_logger_debug` in a hot path costs one call when debug is off.

Do **not** open files under `logs/` yourself, and do not print to the console
for diagnostics — that is what `log_console` is for. A module that links its
own lh (escape hatch) has its own `lh_runtime_logger()`; it is not Authum's.

---

## 4. Host side

`<authum/logger/host.h>` (`src/authum/logs.c`): one channel per loaded module,
opened before `start` and closed after `stop`, plus Authum's own. The module
reaches its channel through `authum_host_t.logger`. Authum's code and lh log
through `lh_runtime_logger()`, which `src/authum/log.cpp` feeds into the
`authum` channel.

---

## Related

- [writing-a-module.md](writing-a-module.md)
- [cfg.md](cfg.md)
- [architecture.md](architecture.md)

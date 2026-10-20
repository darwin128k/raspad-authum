# Logging

Goal: a module developer does not care whether the process is “Metamod →
Authum → module” or anything else. They emit records; the **host**
chooses sinks.

---

## 1. Today (Authum 1.0.0 / 2.0.0 host)

| Channel | Where |
|---------|--------|
| Host | `addons/authum/authum.log` via `lh_runtime_logger` + Metamod `LOG_*` mirror |
| Modules | Optional: call `host->log` / `authum_host_log_line`; and/or their own files |

Policy: [architecture.md](architecture.md). Pure clients (ABI 0.2) get
`authum_host_t` in `start` and call `authum_host_log_line` — no lh.
If a module optionally links its own lh, `lh_runtime_logger()` there is
still **not** `authum.log`.

There is **no** `logs/` directory yet; files sit next to the image.

---

## 2. Target policy

### 2.1 One emit API

Modules call a small host-facing log API. They never open `authum.log`
themselves.

### 2.2 Host scope (three levels)

Configurable on the host (e.g. future `authum.cfg`):

| Scope | `authum.log` contains |
|-------|------------------------|
| `off` | Nothing (or host fatals only) |
| `host` | Authum only (attach, module OK/FAIL, commands) — **no** module chatter |
| `full` | Host + every module line forwarded to the host |

### 2.3 Module-private files

A module may keep its own transcript. That does not force lines into
`authum.log`; forwarding is the host scope above.

### 2.4 Directory layout (future)

```
addons/authum/
  logs/
    authum-….log
  modules/<name>/
    logs/
      <name>-….log
```

---

## 3. Metamod vs Authum module

| | Standalone Metamod plugin | Authum child module |
|--|---------------------------|---------------------|
| Load path | `plugins.ini` | `modules.list` / `modules/` |
| Default log | Own file + console | Host channel + optional own file |

Same mental model: **emit → host policy → sinks**.

---

## Related

- [architecture.md](architecture.md)
- [writing-a-module.md](writing-a-module.md)

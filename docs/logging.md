# Logging

Goal: a module developer does not care whether the process is “Metamod →
Authum → module” or anything else. They emit records; the **host**
chooses sinks.

---

## 1. Today (Authum 1.0.0 line)

| Channel | Where |
|---------|--------|
| Host | `addons/authum/authum.log` via `lh_runtime_logger` + Metamod `LOG_*` mirror |
| qproto (Authum package) | `modules/qproto/qproto.log` + lines mirrored to host through `authum_host_log` |

Policy: [architecture.md](architecture.md). Pure clients (ABI 0.2) get
`authum_host_t` in `start` and call `authum_host_log_line` — no lh.
If a module optionally links its own lh, `lh_runtime_logger()` there is
still **not** `authum.log`.

There is **no** `logs/` directory yet; files sit next to the image.

---

## 2. Target policy (Authum / qproto 2.0 direction)

### 2.1 One emit API

Modules call a small host-facing log API (or `lh_logger_*` on a logger
the host installed). They never open `authum.log` themselves.

### 2.2 Host scope (three levels)

Configurable on the host (e.g. future `authum.cfg`), independent of
PSR-3 severity:

| Scope | `authum.log` contains |
|-------|------------------------|
| `off` | Nothing (or host fatals only) |
| `host` | Authum only (attach, module OK/FAIL, commands) — **no** module chatter |
| `full` | Host + every module line forwarded to the host |

So you can run Vellum/qproto with rich private logs and still keep
`authum.log` clean (`host`), or pull everything into one file (`full`).

### 2.3 Module-private files

A module may still keep its own transcript (qproto’s `LoggingMode` →
file). That does not force lines into `authum.log`; forwarding is the
host scope above.

### 2.4 Directory layout

Prefer a dedicated folder, not the addon root:

```
addons/authum/
  logs/
    authum-YYYYMMDD-HHMMSS.log    # one episode per attach (optional rotation)
  modules/qproto/
    logs/
      qproto-….log                # or single qproto.log — module’s choice
```

Creating `logs/` is **host/module responsibility**. lh’s logger only
writes to an already opened sink. lh today has directory **read** APIs;
mkdir may be added later as a thin OS helper — until then Authum creates
the directory itself.

### 2.5 Episodes

Optional: on Meta_Attach / Meta_Detach open a new file under `logs/`
instead of one endless append. Policy belongs to Authum, not to each
plugin.

---

## 3. Metamod vs Authum

| | Metamod plugin (qproto 1.0.0) | Authum module |
|--|------------------------------|---------------|
| Load path | `plugins.ini` | `modules.list` / `modules/` |
| Default log | Own file + console bits | Host channel + optional own file |
| Vellum | Opt-in in `qproto.cfg` (`EnableVellum`) | Same authorizer rules; logging via host policy |

Same mental model for any third-party module: **emit → host policy →
sinks**.

---

## 4. Related

- [writing-a-module.md](writing-a-module.md) — exports and layout  
- [modules.md](modules.md) — loader  
- [gate.md](gate.md) — auth / connect gate  

---

## 5. Build size / lh link modes

`AUTHUM_LH_LINK_MODE` (CMake cache):

| Value | Meaning |
|-------|---------|
| `needed` (default) | Static `liblh.a`, only referenced objects + `--gc-sections` |
| `whole` | `--whole-archive` — every lh object inside the `.so` |

lh is always a static library (no shared build). Release also
`strip --strip-unneeded` the host image.
`CMAKE_BUILD_TYPE=Release` is `-O3 -DNDEBUG` (not Debug).

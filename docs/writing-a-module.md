# Writing an Authum module

An Authum module is a shared library (`.so` / `.dll`) that the host
`authum_mm` loads. It is **not** a Metamod plugin. Metamod only loads
Authum once; everything else hangs under Authum.

See [architecture.md](architecture.md). Sample: [`examples/hello`](../examples/hello/).

---

## 1. What you export (ABI 0.2)

Include **`#include <authum/client.h>`** only. Do **not** include lh or link
`liblh` unless you opt into the escape hatch.

| Symbol | Type | Role |
|--------|------|------|
| `authum_module_info` | `authum_module_info_t` | Name, SemVer, ABI, … |
| `authum_module_api` | `authum_module_api_t` | `.start(host)` / `.stop()` |

ABI: `AUTHUM_MODULE_ABI` = `0.2.0`.

```c
#include <authum/client.h>

static authum_host_t g_host;

AUTHUM_MODULE_EXPORT authum_module_info_t authum_module_info = {
    .abi = AUTHUM_MODULE_ABI,
    .name = "example",
    .version = AUTHUM_VERSION(0, 1, 0),
    .date = "2026-10-19",
    .author = "you",
    .url = "",
    .logtag = "EXAMPLE",
};

static authum_bool_t
example_start(authum_host_t *host)
{
    if (host == 0)
        return AUTHUM_FALSE;
    g_host = *host;
    authum_host_log_line(&g_host, 0, "[example] start");
    return AUTHUM_TRUE;
}

static void
example_stop(void)
{
    authum_host_log_line(&g_host, 0, "[example] stop");
}

AUTHUM_MODULE_EXPORT const authum_module_api_t authum_module_api = {
    .start = example_start,
    .stop = example_stop,
};
```

---

## 2. Drop-in layout

```
cstrike/addons/authum/
  authum_mm.so
  modules.list                 # one path per line, e.g. example
  modules/example/
    example.so
    example.cfg                # optional
```

`modules.list`: see [modules.md](modules.md). Console: `authum`. Log:
`addons/authum/authum.log`.

---

## 3. Host facade

Children do **not** get `GiveFnptrsToDll`. The host passes `authum_host_t`
into `start` (valid until `stop`):

| Field | Purpose |
|-------|---------|
| `log` | → `authum.log` |
| `engine_funcs` | `void *` → `enginefuncs_t *` |
| `globals` | `void *` → `globalvars_t *` |
| `module_dir` | directory of this module's `.so` / `.dll` (cfg, logs, …) |

Copy the struct (or the function pointers) if you need them after `start` returns.

**Escape hatch:** link your own lh / other library into *your* `.so` for
private utilities. Not the host runtime.

---

## 4. Logging

Use `authum_host_log_line` / `host->log`. Never assume a process-wide
`lh_runtime_logger()` is `authum.log`. Optional private file under your
package is fine. Policy: [logging.md](logging.md).

---

## 5. Build notes

- **32-bit** HLDS (`-m32` on Linux).
- Include path: Authum’s `include/` only (for a pure client).
- Do **not** link `liblh` unless you choose the escape hatch.
- Export visibility: `AUTHUM_MODULE_EXPORT` on both symbols.

---

## 6. Checklist

- [ ] `#include <authum/client.h>` (no lh in a pure client)
- [ ] `authum_module_info.abi` = `0.2.0`, non-null `name`
- [ ] `authum_module_api` exported; `start` returns `AUTHUM_FALSE` only on hard failure
- [ ] Listed in `modules.list` / under `modules/`
- [ ] Folder `foo` → `foo.so` / `foo.dll`
- [ ] Not a Metamod `plugins.ini` entry
- [ ] Logs via host facade

Loader details: [modules.md](modules.md). Gate: [gate.md](gate.md).

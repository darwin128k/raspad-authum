# Writing an Authum module

An Authum module is a shared library (`.so` / `.dll`) that the host
`authum_mm` loads. It is **not** a Metamod plugin. Metamod only loads
Authum once; everything else hangs under Authum.

Authum does not ship production child modules in the host binary. A minimal
working sample is [`examples/hello`](../examples/hello/). Ship your own
`.so` / `.dll` under `addons/authum/modules/<name>/` (see qproto as a
separate repo for a full auth example).

---

## 1. What you export

Exactly two **unmangled C** symbols (`AUTHUM_MODULE_EXPORT`,
`visibility("default")` on Linux):

| Symbol | Type | Role |
|--------|------|------|
| `authum_module_info` | `authum_module_info_t` | Name, SemVer, ABI, author, … |
| `authum_module_ops` | `lh_os_module_ops_t` | `.start` / `.stop` |

Without `authum_module_ops`, the image is opened and discarded.
Without a valid `authum_module_info` (or wrong ABI), `start` fails and
the module is unloaded.

ABI today: `AUTHUM_MODULE_ABI` = `0.1.0`
(`lh_version_is_compatible` against the host).

Minimal skeleton:

```c
#include <authum/module.h>
#include <authum/module/info.h>
#include <lh/version/initializer.h>

AUTHUM_MODULE_EXPORT authum_module_info_t authum_module_info = {
    .abi = AUTHUM_MODULE_ABI,
    .name = "example",
    .version = lh_version_initializer(0, 1, 0),
    .author = "you",
    .logtag = "EXAMPLE",
};

static lh_bool_t
example_start(authum_module_t *self)
{
    (void)self;
    /* take info, bind host APIs, do work */
    return lh_bool_true;
}

static void
example_stop(authum_module_t *self)
{
    (void)self;
}

AUTHUM_MODULE_EXPORT const lh_os_module_ops_t authum_module_ops = {
    .start = example_start,
    .stop = example_stop,
};
```

You may point `.start` / `.stop` at the host’s `authum_module_start` /
`authum_module_stop` if you only need identity + nested `modules/` and
no custom work.

---

## 2. Drop-in layout

```
cstrike/addons/authum/
  authum_mm.so
  modules.list                 # one path per line, e.g. example
  modules/example/
    example.so                 # or example.dll — Authum appends the OS suffix
    example.cfg                # optional, your format
```

`modules.list` dialect (not INI): see [modules.md](modules.md). Relative
entries resolve under `modules/`. Folder name `example` →
`modules/example/example.so`.

List the module:

```
example
```

Restart HLDS (or reload Authum). Console: `authum`. Host log:
`addons/authum/authum.log` (and Metamod server log).

---

## 3. Talking to the host

Children do **not** get `GiveFnptrsToDll`. Use the host facade
([`authum/host.h`](../include/authum/host.h)) — see [architecture.md](architecture.md).

```c
#include <authum/host.h>

static authum_host_t g_host;

static lh_bool_t
example_start(authum_module_t *self)
{
    if (!authum_host_bind(self, &g_host))
        return lh_bool_false;
    authum_host_log_line(&g_host, 0, "[example] start");
    /* engfuncs: (enginefuncs_t *)g_host.engine_funcs(); */
    return lh_bool_true;
}
```

| Host symbol | Purpose |
|-------------|---------|
| `authum_host_log` | line → `authum.log` |
| `authum_engine_funcs` | `void *` → `enginefuncs_t *` |
| `authum_globals` | `void *` → `globalvars_t *` |

Do not call sibling modules by name. There is no shared plugin table
between children — only the parent facade.

**Escape hatch:** link your own static lh (Authum’s `lib/lh`) or any other
library into *your* `.so` for private code. That is not the host runtime.

---

## 4. Logging (what you should do)

**Write lines through the host; do not pick the sink.**

- Use `authum_host_log_line` (or `g_host.log`). Never assume
  `lh_runtime_logger()` in the module is `authum.log`.
- Optional: your own file under **your** package (e.g.
  `modules/example/logs/…`) if the module needs a private transcript.
- Whether those lines also appear in `authum.log` is **Authum policy**
  (see [logging.md](logging.md): off / host-only / full).

qproto 1.0.0 (Metamod-only release) keeps its own `qproto.log` +
`LoggingMode`. The Authum-hosted line should feed the host logger so
Metamod vs Authum is invisible to you.

---

## 5. Build notes

- Target: **32-bit** HLDS (`-m32` on Linux).
- Link against Authum headers (`include/authum/…`) and lh (Authum’s
  `lib/lh` submodule, branch `0.4.0`).
- HLSDK / Metamod headers only if you call engfuncs types.
- Export visibility: use `AUTHUM_MODULE_EXPORT` on both symbols.

---

## 6. Checklist before shipping

- [ ] `authum_module_info.name` non-null, `abi` = `0.1.0`
- [ ] `authum_module_ops` exported; `start` returns false only on hard failure
- [ ] Listed in `modules.list` (or discoverable under `modules/`)
- [ ] Native name: folder `foo` → `foo.so` / `foo.dll` inside
- [ ] No dependency on being a Metamod `plugins.ini` entry
- [ ] Logs go through host API (plus optional private file)
- [ ] Configs and data live beside the module image, not in `cstrike/` root

Deeper loader semantics: [modules.md](modules.md). Connect/auth gate:
[gate.md](gate.md).

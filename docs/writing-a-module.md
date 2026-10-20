# Writing an Authum module

An Authum module is a shared library (`.so` / `.dll`) that the host
`authum_mm` loads. It is **not** a Metamod plugin. Metamod only loads
Authum once; everything else hangs under Authum.

See [architecture.md](architecture.md). Sample: [`examples/hello`](../examples/hello/).

### Language requirement

Authum **modules MUST be pure C** (C11 recommended) with **strict snake_case**
identifiers (functions, types, struct fields, globals). Do not ship module code
as C++ (no classes, namespaces, templates, references, `nullptr`, C++ headers).
The host Metamod plugin (`authum_mm`) may remain C++ / mixed; only the loadable
child modules are constrained to C + snake_case. Config *file* keys (e.g.
`EnableRevEmu` for dproto compatibility) may keep their historical spellings.

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
| `engine` | `const authum_engine_t *` — game dir / server print / server command (no HLSDK) |
| `engine_funcs` / `globals` | escape hatch: `void *` → `enginefuncs_t *` / `globalvars_t *` |
| `module_dir` | directory of this module's `.so` / `.dll` (cfg, logs, …) |
| `crypto` | `const authum_crypto_t *` — Rijndael + SHA session API (lh subset; see `<authum/crypto.h>`) |

Crypto mirrors lh layout (`authum/crypto/rijndael.h`, `…/sha.h`, mode/kind leaves) but only the
session surface is exposed. Engine likewise exposes only the subset modules need:

```c
char game_dir[256];
authum_engine_get_game_dir(host->engine, game_dir, sizeof game_dir);
authum_engine_server_print(host->engine, "[mod] hello\n");
authum_engine_server_command(host->engine, "kick #1\n");

authum_crypto_rijndael_t ctx;
authum_crypto_rijndael_init(host->crypto, &ctx, key, 32, 0, 0, 32,
                            authum_crypto_rijndael_mode_ecb);
authum_crypto_rijndael_decrypt_block(host->crypto, &ctx, in, out);

unsigned char dig[AUTHUM_CRYPTO_SHA256_DIGEST_SIZE];
authum_crypto_sha_hash(host->crypto, authum_crypto_sha_kind_256, data, len, dig, sizeof dig);
```

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

- **Pure C** sources (`.c` / C headers with include guards); `project(... C)` is enough — no CXX.
- **snake_case** for all C identifiers in the module.
- **32-bit** HLDS (`-m32` on Linux).
- Include path: Authum’s `include/` only (for a pure client).
- Do **not** link `liblh` unless you choose the escape hatch.
- Export visibility: `AUTHUM_MODULE_EXPORT` on both symbols.

---

## 6. Checklist

- [ ] Pure C11 module (no `.cpp` / C++ in the module)
- [ ] Strict snake_case for all C identifiers
- [ ] `#include <authum/client.h>` (no lh in a pure client)
- [ ] `authum_module_info.abi` = `0.2.0`, non-null `name`
- [ ] `authum_module_api` exported; `start` returns `AUTHUM_FALSE` only on hard failure
- [ ] Listed in `modules.list` / under `modules/`
- [ ] Folder `foo` → `foo.so` / `foo.dll`
- [ ] Not a Metamod `plugins.ini` entry
- [ ] Logs via host facade

Loader details: [modules.md](modules.md). Gate: [gate.md](gate.md).

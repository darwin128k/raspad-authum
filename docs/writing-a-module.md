# Writing an Authum module

An Authum module is a shared library (`.so` / `.dll`) that the host
`authum_mm` loads. It is **not** a Metamod plugin. Metamod only loads
Authum once; everything else hangs under Authum.

See [architecture.md](architecture.md). Templates to copy: [`examples/`](../examples/) — start from
[`skeleton`](../examples/skeleton/), take pieces from the topic examples.

### Language requirement

Authum **modules MUST be pure C** (C11 recommended) with **strict snake_case**
identifiers (functions, types, struct fields, globals), and must use **Authum’s
own types** (`authum_bool_t`, `authum_usize_t`, `authum_byte_t`, `authum_ptr`,
…) — not raw libc/`size_t` on the Authum API surface, and never `<lh/…>`.
Do not ship module code as C++ (no classes, namespaces, templates, references,
`nullptr`, C++ headers). The host Metamod plugin (`authum_mm`) may remain C++ /
mixed; only the loadable child modules are constrained to C + snake_case.
Config *file* keys use snake_case matching C fields (e.g. `enable_revemu`,
`max_bans`) — not PascalCase.

**Naming / enums / types:** full rules in [module-style.md](module-style.md).
In short — type-as-namespace like lh (`qproto_log_mode_console`, not
`QPROTO_LOG_CONSOLE` or `rl_console`); typedefs end in `_t`, tags do not.

---

## 1. What you export (ABI 1.0)

Include **`#include <authum/client.h>`** only. Do **not** include lh or link
`liblh` unless you opt into the escape hatch.

| Symbol | Type | Role |
|--------|------|------|
| `authum_module_info` | `authum_module_info_t` | Name, SemVer, ABI, … |
| `authum_module_api` | `authum_module_api_t` | `.start(host)` / `.stop()` |

ABI: `AUTHUM_MODULE_ABI` = `1.0.0`. Every table the host passes starts with
`size` and only grows at the end, so a 1.x module loads on any 1.x host: a
function the host is too old for answers "not available" (false / 0 / empty
string) instead of crashing. Only a major bump means rebuilding. See
`<authum/abi.h>`.

```c
#include <authum/client.h>

authum_host_t g_example_host;

AUTHUM_MODULE_EXPORT authum_module_info_t authum_module_info = {
    .abi = AUTHUM_MODULE_ABI,
    .name = "example",
    .version = AUTHUM_VERSION(0, 1, 0),
    .date = "2026-10-19",
    .author = "you",
    .url = "",
    .logtag = "EXAMPLE",
};

authum_bool_t
example_start(authum_host_t *host)
{
    if (authum_null_eq(host))
        return authum_bool_false;
    authum_host_copy(&g_example_host, host);
    authum_host_log_line(&g_example_host, 0, "[example] start");
    return authum_bool_true;
}

void
example_stop(void)
{
    authum_host_log_line(&g_example_host, 0, "[example] stop");
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

`modules.list`: see [modules.md](modules.md). Console: `authum`. Logs:
`addons/authum/logs/<module>/YYYY-MM-DD.log`, levels in `addons/authum/authum.cfg`
([logging.md](logging.md)).

---

## 3. Host facade

Children do **not** get `GiveFnptrsToDll`. The host passes `authum_host_t`
into `start` (valid until `stop`). Keep it with `authum_host_copy(&g_host, host)`
— never `g_host = *host`, which reads past the table of an older host. Prefer
these accessors to the `engine_funcs` escape hatch: what goes through them
survives Authum's internal changes.

| Accessor | Purpose |
|----------|---------|
| `authum_logger_*` / `authum_host_log_line` | the module's log — [logging.md](logging.md) |
| `authum_host_engine` / game dir / print / command | engine facade (no HLSDK) |
| `authum_host_engine_funcs` / `authum_host_globals` | escape hatch → HLSDK types |
| `authum_host_env` / `authum_host_env_module` / `_game` / … | `authum_path_t` roots |
| `authum_host_env_module_cstr` | deprecated C string of module dir |
| `authum_host_crypto` | Rijndael + SHA (`<authum/crypto.h>`) |
| `authum_host_sandbox` | Module memory freed at unload — [sandbox.md](sandbox.md) |
| `authum_host_events` / `authum_host_subscribe` / `authum_host_publish` | Event bus — [events.md](events.md) |
| `authum_host_kick` | `kick #userid "reason"` (sanitized) |
| `authum_host_max_clients` / `_game_time` / `_get_map_name` | current map |
| `authum_host_cvar_get` / `_cvar_set` | server cvars |
| `authum_host_client_is_connected` / `_userid` / `_index_by_userid` | player slots (entity index 1 … maxClients, as in `authum.client.*`) |
| `authum_host_client_get_name` / `_auth_id` / `_address` / `_info` | who the player is (`_info` reads `setinfo` keys) |
| `authum_host_client_print` | text to one player or everyone (`authum_engine_print_console` / `_center` / `_chat`) |
| `authum_host_client_command` | run a command in the player's console |
| `authum_host_query_cvar` | Ask a client for a cvar; the answer is `authum.client.cvar_value` — [events.md](events.md) |
| `authum_host_command_add[_all]` | `authum <module> <name>` console / RCON commands — [commands.md](commands.md) |

Paths in `env` are absolute when the host can resolve them from the module image
(current volume, native separators). Prefer `authum_host_env_module(host)` for
cfg/logs; `authum_path_join` appends a leaf.

**Config:** defaults on `g_mod_config = { … }`, then
`authum_cfg_apply_items` from an `authum_cfg_item_t[]`. Keys that need a custom
parser or allow-list stay **outside** the table — see **[cfg.md](cfg.md)**
(full dialect, `get_*` vs `apply_*`, custom-key recipes).

**Linking:** put Authum under `lib/authum` (submodule), then:

```cmake
add_subdirectory(lib/authum/client)
target_link_libraries(mymod PRIVATE authum_client)
```

Do not compile Authum `.c` files into the module. Do not invent
`AUTHUM_SRC_DIR` heuristics that point at a sibling checkout.

Never touch `host->…` fields — only `authum_host_*` getters/setters.

```c
authum_char_t game_dir[256];
authum_host_get_game_dir(host, game_dir, sizeof game_dir);
authum_host_server_print(host, "[mod] hello\n");
authum_host_server_command(host, "kick #1\n");

const authum_crypto_t *crypto = authum_host_crypto(host);
authum_crypto_rijndael_t ctx;
authum_crypto_rijndael_init(crypto, &ctx, key, 32, 0, 0, 32,
                            authum_crypto_rijndael_mode_ecb);
authum_crypto_rijndael_decrypt_block(crypto, &ctx, in, out);

unsigned char dig[AUTHUM_CRYPTO_SHA256_DIGEST_SIZE];
authum_crypto_sha_hash(crypto, authum_crypto_sha_kind_256, data, len, dig, sizeof dig);
```

Copy the struct (or the function pointers) if you need them after `start` returns.

**Escape hatch:** link your own lh / other library into *your* `.so` for
private utilities. Not the host runtime.

---

## 4. Logging

Use **`authum_logger_t`** (`<authum/logger.h>`): `authum_logger_init(&log,
&g_host)` in `start`, then `authum_logger_info` / `_warning` / `_debug` / ….
The module only picks the level; Authum writes `logs/<module>/YYYY-MM-DD.log`
and copies warnings and errors to its own log, as `authum.cfg` says. No
logging keys in the module's cfg, no files of its own, no console prints for
diagnostics. Details: [logging.md](logging.md).

---

## 5. Build notes

- **Pure C** sources (`.c` / C headers with include guards); `project(... C)` is enough — no CXX.
- **snake_case** for all C identifiers in the module.
- **32-bit** HLDS (`-m32` on Linux).
- **Submodule** Authum at `lib/authum`; link `authum_client` (PUBLIC includes).
- Do **not** link `liblh` unless you choose the escape hatch.
- Export visibility: `AUTHUM_MODULE_EXPORT` on both symbols.
- Config API: [cfg.md](cfg.md).

---

## 6. Checklist

- [ ] Pure C11 module (no `.cpp` / C++ in the module)
- [ ] Strict snake_case; enums `{type}_leaf` ([module-style.md](module-style.md))
- [ ] Authum foundation types on Authum / public ABI
- [ ] `#include <authum/client.h>` (no lh in a pure client)
- [ ] `authum_module_info.abi` = `AUTHUM_MODULE_ABI` (`1.0.0`); host kept with `authum_host_copy`, non-null `name`
- [ ] `authum_module_api` exported; `start` returns `authum_bool_false` only on hard failure
- [ ] Listed in `modules.list` / under `modules/`
- [ ] Folder `foo` → `foo.so` / `foo.dll`
- [ ] Not a Metamod `plugins.ini` entry
- [ ] Logs via host facade
- [ ] Long-lived memory from the sandbox ([sandbox.md](sandbox.md))
- [ ] Talks to other modules only via events ([events.md](events.md)); no Metamod hooks of its own for player connect/disconnect
- [ ] Console / RCON commands via `authum_host_command_add` — never `pfnAddServerCommand` ([commands.md](commands.md))
- [ ] Config: defaults + `authum_cfg_apply_items`; custom keys after the table ([cfg.md](cfg.md))
- [ ] Links `authum_client` from `lib/authum/client` (no Authum `.c` in the module)

Loader details: [modules.md](modules.md). Gate: [gate.md](gate.md). Config: [cfg.md](cfg.md).

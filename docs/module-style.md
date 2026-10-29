# Authum module coding style

Rules for **child modules** loaded by `authum` (not for the host image
itself). Same spirit as lh: snake_case, type-as-namespace, Authum’s own types
on the Authum surface. Sample: [`examples/hello`](../examples/hello/).

---

## 1. Language

| Rule | Detail |
|------|--------|
| Pure C | C11. No `.cpp`, classes, namespaces, templates, references, `nullptr` |
| snake_case | Functions, types, fields, globals, enum constants |
| No lh in pure clients | `#include <authum/client.h>` only; never `<lh/…>` unless escape hatch |
| Includes | Only `#include <…>` — never `"qproto/foo.h"` / `"bridge.h"` |
| Authum types | Paths → `authum_path_t`; cfg → defaults on the struct + `authum_cfg_apply_items` (or manual `get_*` / `apply_*`); no raw `atoi`/`strncpy`/`strcasecmp` |
| Host access | Never `host->engine` / `host->env` / … — only `authum_host_*` getters/setters |
| Bricks, not statics | No `static` functions and no `static inline` in headers: every function is a small, prefixed, linkable brick declared in a header (debuggable, testable). `static` stays only for file-local data |
| Small modules | A module holds only its own logic. Engine access, memory lifetime, config parsing, inter-module talk, console commands are Authum's job (host facade, sandbox, cfg, events, commands) |
| No engine commands | `authum <module> <name>` via [commands.md](commands.md); never `pfnAddServerCommand` (clashes with AMXX / others, crashes after unload) |

Config *file* keys use the same snake_case as C fields (`enable_revemu`, `max_bans`, …).

---

## 2. Names: module prefix

Every **public** identifier starts with the module name:

```c
qproto_get_client_authkind   /* good */
GetClientAuthKind            /* bad — C++ / Metamod era */
```

Prefer the same prefix on **internal** types too (`qproto_client_auth_kind_t`),
so the tree does not mix bare `client_auth_kind_t` with `qproto_*`.

---

## 3. Types (`_t`) and tags

| Kind | Tag / name | Typedef |
|------|------------|---------|
| Struct / enum | `qproto_log_mode` (no `_t`) | `qproto_log_mode_t` |
| Opaque pointer helpers | — | `authum_ptr` (Authum) |

```c
enum qproto_log_mode {
    qproto_log_mode_none = 0,
    qproto_log_mode_console = 1,
    qproto_log_mode_logfile = 2,
    qproto_log_mode_host = 4,
};
typedef enum qproto_log_mode qproto_log_mode_t;
```

Anonymous `typedef enum { … } foo_t;` is discouraged — the tag is the
namespace for constants.

Small selectors (like Authum crypto kind/mode) may be `#define` constants +
`typedef authum_byte_t foo_t` instead of an `enum`, still with `foo_*` names.

---

## 4. Enum / flag constants

**Constant name = type name without `_t` + `_` + leaf.**

| Good | Bad |
|------|-----|
| `qproto_auth_kind_steam` | `QPROTO_AUTH_STEAM` |
| `qproto_key_kind_hddsn` | `QPROTO_KEY_HDDSN` |
| `qproto_log_mode_console` | `rl_console` |
| `authum_crypto_sha_kind_256` | `SHA256` |

- No SCREAMING_SNAKE for enum values (that style is for preprocessor macros).
- No short invented prefixes (`rl_`, `CA_`, `AK_`).
- Bitmasks still use the same names (`qproto_log_mode_console \| qproto_log_mode_logfile`).

### Macros: the case says how a macro behaves

| Case | When | Authum examples |
|------|------|-----------------|
| `SCREAMING_SNAKE` | Not usable as an expression: an initializer, a declaration or attribute, a constant, an include guard | `AUTHUM_VERSION(1, 0, 0)` (expands to `{…}`), `AUTHUM_MODULE_ABI`, `AUTHUM_MODULE_EXPORT`, `AUTHUM_EXTERN_C_BEGIN`, `AUTHUM_CRYPTO_SHA256_DIGEST_SIZE`, `QPROTO_FOO_H` |
| `snake_case` | Behaves like a value or a call | `authum_null`, `authum_null_eq(p)`, `authum_bool_true`, `authum_unused(x)` |

Capitals warn the reader that this is not a function: `AUTHUM_VERSION(1, 0, 0)`
only fits where a `{…}` initializer does. lh follows the same split
(`lh_null_eq` vs `LH_COMPILER_EXTERN_C_BEGIN`).

---

## 5. Logging

Use `<authum/logger.h>` (`authum_logger_info`, sinks, levels). Do not ship a
private `mymod_log()` that reimplements host/console/file unless you need a
different design — see [logging.md](logging.md).

---

## 6. Authum foundation types

On the Authum API surface (and preferably inside the module) use:

| Use | Type / symbol |
|-----|----------------|
| Boolean | `authum_bool_t`, `authum_bool_true` / `authum_bool_false` |
| Size / length | `authum_usize_t` |
| Byte / binary | `authum_byte_t` / `authum_u8_t` |
| 32-bit | `authum_u32_t`, `authum_i32_t` |
| Text element | `authum_char_t` |
| Untyped pointer | `authum_ptr` (`void *`), `authum_cptr` (`const void *` — not `const authum_ptr`, which is `void *const`) |
| Null | `authum_null`, `authum_null_eq(p)` |
| Unused parameter | `authum_unused(user);` — not `(void)user;` |

Module structs should use these, not raw `int` / `uint32_t` / `size_t` /
`uint8_t`, except where an external ABI (libc, peer plugins) forces otherwise.

What each type is, and its `printf` format — no cast needed:

| Type | C type | Format |
|------|--------|--------|
| `authum_i32_t` | `int` | `%d` |
| `authum_u32_t`, `authum_usize_t` | `unsigned int` | `%u` |
| `authum_i64_t` / `authum_u64_t` | `long long` / `unsigned long long` | `%lld` / `%llu` |
| `authum_bool_t`, `authum_byte_t`, `authum_u8_t` | `unsigned char` | `%u` (promoted) |
| `authum_char_t` | `char` | `%c`, `%s` for a string |
| `authum_float_t` | `float` | `%f` (promoted) |

A cast is for a real conversion only — `(int)health` turns a `float` into an
integer; `(int)ev->index` on an `authum_i32_t` is noise.

Do not put raw `size_t` / `void *` / `NULL` into Authum-facing signatures.
Libc types remain OK for FILE*, `snprintf`, private buffers, etc.

Host helpers:

```c
authum_host_copy_game_dir(host, buf, (authum_usize_t)sizeof buf);
authum_crypto_sha_hash(authum_host_get_crypto(host), authum_crypto_sha_kind_256,
                       (authum_ptr)data, (authum_usize_t)len,
                       (authum_ptr)out, (authum_usize_t)out_size);
```

---

## 7. Headers and layout

- Include guards: `QPROTO_FOO_H` / `AUTHUM_FOO_H` (macros — SCREAMING OK).
- C linkage for C++ readers of the header: `AUTHUM_EXTERN_C_BEGIN` /
  `AUTHUM_EXTERN_C_END` (`<authum/extern.h>`), not the
  `#ifdef __cplusplus` / `extern "C" {` pair.
- One concept per header when practical (`…/kind.h`, `…/mode.h`), lh-style.
- **Path is the namespace:** `qproto/api.h`, not `qproto/qproto_api.h`.
  Compound names are directories: `qproto/client/state.h`, not
  `qproto/client_state.h`. Symbols stay `qproto_*`; file names do not
  repeat the module folder.
- Export only `authum_module_info` + `authum_module_api`. Peer modules reach
  you through events; publish their names and payload structs in a
  types-only header (`qproto/event.h`). See [events.md](events.md).

---

## 8. Quick checklist

- [ ] Pure C11, snake_case
- [ ] Module prefix on public (and ideally internal) names
- [ ] Enum/struct tag without `_t`; typedef with `_t`
- [ ] Constants `{tag}_leaf`, not `MODULE_LEAF` / `rl_leaf`
- [ ] Authum types on Authum / public module ABI; casts only for real conversions
- [ ] `authum_unused(x)` for unused parameters; `AUTHUM_EXTERN_C_BEGIN` in headers
- [ ] Logging via `authum_logger_*` (or explicit bricks), not a bespoke clone
- [ ] No `<lh/…>` in a pure client
- [ ] No `static` functions; module-prefixed bricks declared in headers

See also [writing-a-module.md](writing-a-module.md), [architecture.md](architecture.md).

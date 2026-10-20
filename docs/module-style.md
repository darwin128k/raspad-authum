# Authum module coding style

Rules for **child modules** loaded by `authum_mm` (not for the Metamod host
itself). Same spirit as lh: snake_case, type-as-namespace, Authum’s own types
on the Authum surface. Sample: [`examples/hello`](../examples/hello/).

---

## 1. Language

| Rule | Detail |
|------|--------|
| Pure C | C11. No `.cpp`, classes, namespaces, templates, references, `nullptr` |
| snake_case | Functions, types, fields, globals, enum constants |
| No lh in pure clients | `#include <authum/client.h>` only; never `<lh/…>` unless escape hatch |

Config *file* keys may keep historical spellings (`EnableRevEmu`, …).

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

`SCREAMING` macros are fine for: include guards, `QPROTO_API_VERSION_*`,
sizes (`AUTHUM_CRYPTO_SHA256_DIGEST_SIZE`), export attributes.

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
| Byte / binary | `authum_byte_t` |
| Text element | `authum_char_t` |
| Untyped pointer | `authum_ptr` |
| Null | `authum_null`, `authum_null_eq(p)` |

Do not put raw `size_t` / `void *` / `NULL` into Authum-facing signatures.
Libc types remain OK for FILE*, `snprintf`, private buffers, etc.

Host helpers:

```c
authum_engine_get_game_dir(host->engine, buf, (authum_usize_t)sizeof buf);
authum_crypto_sha_hash(host->crypto, authum_crypto_sha_kind_256,
                       (authum_ptr)data, (authum_usize_t)len,
                       (authum_ptr)out, (authum_usize_t)out_size);
```

---

## 7. Headers and layout

- Include guards: `QPROTO_FOO_H` / `AUTHUM_FOO_H` (macros — SCREAMING OK).
- One concept per header when practical (`…/kind.h`, `…/mode.h`), lh-style.
- Export only `authum_module_info` + `authum_module_api` to Authum; other
  symbols for peer modules via your own stable C API if needed.

---

## 8. Quick checklist

- [ ] Pure C11, snake_case
- [ ] Module prefix on public (and ideally internal) names
- [ ] Enum/struct tag without `_t`; typedef with `_t`
- [ ] Constants `{tag}_leaf`, not `MODULE_LEAF` / `rl_leaf`
- [ ] Authum types on Authum / public module ABI
- [ ] Logging via `authum_logger_*` (or explicit bricks), not a bespoke clone
- [ ] No `<lh/…>` in a pure client

See also [writing-a-module.md](writing-a-module.md), [architecture.md](architecture.md).

# Module configuration (`authum_cfg`)

Flat `key = value` files for Authum client modules. Implementation lives in the
**`authum_client`** static library (`src/client/cfg.c`). Modules
`#include <authum/cfg.h>` and **link** `authum_client` — they never compile
Authum `.c` files into themselves.

Related headers: `<authum/cfg.h>`, `<authum/cfg/item.h>`, `<authum/cfg/kind.h>`,
`<authum/cstr.h>`, `<authum/path.h>`.

---

## 1. File dialect

| Rule | Behavior |
|------|----------|
| Line form | `key = value` (spaces around `=` optional) |
| Split | First `=` separates key and value; value may contain more `=` |
| Trim | Leading/trailing whitespace on line, key, and value |
| Comments | Lines starting with `#` or `;` (after trim) are skipped |
| Blank lines | Skipped |
| Sections | No `[sections]` — one flat namespace |
| Keys | Lookup is **case-insensitive** |

Bool values accept common spellings (`1` / `0`, `true` / `false`, `yes` / `no`,
`on` / `off`, case-insensitive) via `authum_cstr_parse_bool`.

Missing file → keep your in-memory defaults. Missing key → leave that field
unchanged (apply) or use the explicit default (get).

---

## 2. Link `authum_client` (do not compile Authum sources)

Standard layout for a module repo (same idea as Authum’s `lib/lh`):

```
mymod/
  lib/authum/          # git submodule → raspad-authum
  CMakeLists.txt
  src/…
```

```cmake
add_subdirectory(lib/authum/client)   # defines target authum_client
# …
target_link_libraries(mymod PRIVATE authum_client)
# PUBLIC include of authum_client already exposes Authum’s include/
```

`client/CMakeLists.txt` builds a PIC static lib (optional `-m32` via
`AUTHUM_CLIENT_FORCE_32BIT`, default ON for HLDS modules).

Wrong:

- `"${SOME_PATH}/src/client/cfg.c"` in the module’s `add_library`
- inventing a parallel ini/cfg parser when this one fits

---

## 3. Recommended pattern: defaults + `apply_items`

1. Put **all defaults** on a global (or owned) config object via static
   initialization.
2. Declare a table of plain key → field bindings (`authum_cfg_item_t[]`).
3. Open the file, call `authum_cfg_apply_items`, close.
4. Handle **odd keys** after the table (see §5).

```c
#include <authum/cfg.h>
#include <authum/host.h>
#include <authum/path.h>

struct mymod_config {
    authum_bool_t enable_foo;
    authum_u32_t  logging_sinks;
    authum_char_t greeting[64];
};
typedef struct mymod_config mymod_config_t;

mymod_config_t g_mymod_config = {
    .enable_foo = authum_bool_true,
    .logging_sinks = 2u,
    .greeting = "hello",
};

static const authum_cfg_item_t g_mymod_config_items[] = {
    { "EnableFoo", authum_cfg_kind_bool, &g_mymod_config.enable_foo, 0 },
    { "LoggingMode", authum_cfg_kind_u32, &g_mymod_config.logging_sinks, 0 },
    { "Greeting", authum_cfg_kind_cstr, g_mymod_config.greeting,
      sizeof g_mymod_config.greeting },
};

void
mymod_config_load(authum_host_t *host)
{
    authum_path_t path;
    authum_cfg_t cfg;

    authum_path_clear(&path);
    if (!authum_path_set(&path, authum_host_env_module(host)) ||
        !authum_path_join(&path, "mymod.cfg") ||
        !authum_cfg_open(&cfg, &path)) {
        /* defaults already on g_mymod_config */
        return;
    }

    authum_cfg_apply_items(&cfg, g_mymod_config_items,
                           (authum_usize_t)(sizeof g_mymod_config_items /
                                            sizeof g_mymod_config_items[0]));
    authum_cfg_close(&cfg);
}
```

Kinds (`authum_cfg_kind_t`):

| Kind | Destination | `size` field |
|------|-------------|--------------|
| `authum_cfg_kind_bool` | `authum_bool_t *` | ignored |
| `authum_cfg_kind_i32` | `authum_i32_t *` | ignored |
| `authum_cfg_kind_u32` | `authum_u32_t *` | ignored |
| `authum_cfg_kind_cstr` | `authum_char_t *` buffer | capacity including NUL |

`apply_*` / `apply_item` / `apply_items` only write when the key is present and
parses. They do **not** wipe fields on missing keys — that is why defaults on
the object matter.

---

## 4. One-shot reads: `get_*`

Use when you need a value once with an explicit fallback, without mutating a
long-lived struct:

```c
authum_bool_t on = authum_cfg_get_bool(&cfg, "EnableFoo", authum_bool_true);
authum_i32_t  n  = authum_cfg_get_i32(&cfg, "MaxPlayers", 32);
authum_char_t buf[64];
authum_cfg_get_cstr(&cfg, "Name", buf, sizeof buf, "default");
```

Prefer `apply_*` / the items table for normal module settings.

---

## 5. Custom keys (cannot go through the table “all at once”)

The items table only knows bool / i32 / u32 / cstr. Keep using it for the
boring majority. For anything else, **open once**, run `apply_items`, then
handle specials with `apply_*` / `get_*` and your own parse / validate.

Typical cases:

| Situation | Approach |
|-----------|----------|
| Enum / string layout (`auto`, `classic`, …) | `apply_cstr` into a scratch buffer, then map with `authum_cstr_eq(..., ignore_case)` |
| Restricted integer set | `apply_i32`, accept only allowed values; ignore illegal |
| Dependent keys | Read A, then decide whether/how to apply B |
| Non-scalar / multi-token value | `get_value` / `apply_cstr`, parse yourself |
| Different file / format | Don’t force this dialect — open that file separately |

Sketch (same open as §3):

```c
authum_char_t layout[32];
authum_i32_t cid;

authum_cfg_apply_items(&cfg, items, count);

if (authum_cfg_apply_cstr(&cfg, "EngineLayout", layout, sizeof layout))
    g.cfg.layout = mymod_parse_layout(layout); /* your mapper */

cid = (authum_i32_t)g.cfg.cid_steam;
if (authum_cfg_apply_i32(&cfg, "cid_Steam", &cid)) {
    if (cid == ALLOWED_A || cid == ALLOWED_B)
        g.cfg.cid_steam = (mymod_cid_t)cid;
    /* else keep default — do not accept garbage */
}
```

Reference implementation: qproto’s `qproto_config_load` — bulk
`qproto_config_items[]`, then hand-parsed `EngineLayout` and validated
`cid_Steam`.

Do **not** put parsers in header `static inline` helpers just to “save a .c”:
keep load logic in one `.c` so it stays debuggable.

---

## 6. Open helpers

| Call | Use |
|------|-----|
| `authum_cfg_open(cfg, path)` | `authum_path_t *` (preferred with host env) |
| `authum_cfg_open_cstr(cfg, path)` | C string path |
| `authum_cfg_adopt(cfg, FILE *)` | Already-open stream (module owns close via `authum_cfg_close`) |
| `authum_cfg_close(cfg)` | Always pair with a successful open/adopt |

Each `get_*` / `apply_*` rewinds and scans the file for the key. That is fine
for small module configs; do not treat this as a high-frequency hot path.

Value pointers from `authum_cfg_get_value` live in `cfg->line` until the next
lookup — copy if you need them longer.

---

## 7. Paths for cfg / logs

Prefer host-filled roots, not ad-hoc `char *` or `getcwd`:

```c
const authum_path_t *mod = authum_host_env_module(host);
authum_path_t path;
authum_path_set(&path, mod);
authum_path_join(&path, "mymod.cfg");
```

Never read `host->env` / `host->engine` fields directly — only
`authum_host_*` getters. Details: [writing-a-module.md](writing-a-module.md),
`<authum/path.h>`, `<authum/env.h>`.

---

## 8. Checklist

- [ ] Defaults on the config object (static init)
- [ ] Plain keys in `authum_cfg_item_t[]` + `authum_cfg_apply_items`
- [ ] Custom / validated keys after the table with `apply_*` + your parse
- [ ] Link `authum_client`; do not compile Authum `.c` into the module
- [ ] Includes only as `#include <authum/…>`
- [ ] Missing file → silent defaults (or one log line), not a hard start failure

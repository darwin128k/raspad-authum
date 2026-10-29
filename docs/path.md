# Paths (`authum_path_t`) and host env

Modules must not invent `char *data_dir` / `getcwd` for Authum roots. The host
fills `authum_env_t` with fixed-capacity `authum_path_t` directories; modules
join leaves onto a copy.

Headers: `<authum/path.h>`, `<authum/env.h>`, `<authum/host.h>`.

---

## Env roots

| Getter | Typical use |
|--------|-------------|
| `authum_host_get_game_path` | Game directory (e.g. `cstrike`) |
| `authum_host_env_addons` | `…/addons` |
| `authum_host_env_authum` | `…/addons/authum` |
| `authum_host_env_modules` | `…/addons/authum/modules` |
| `authum_host_get_module_path` | `…/modules/<module>` — the module's own folder |
| `authum_host_get_module_config_path` | `…/modules/<module>/configs` — the admin's |
| `authum_host_get_module_lang_path` | `…/modules/<module>/lang` — translations |
| `authum_host_get_module_data_path` | `…/modules/<module>/data` — what the module writes |
| `authum_host_env_models` / `_sprites` | Content trees when needed |

Always via `authum_host_*` — never `host->env->…`.

Environment paths describe locations; filling the environment does not create
`data`. Reading an optional file must not create its directory. Before writing,
the module calls `authum_path_ensure_dir` for the required parent. This delegates
to LH's filesystem API. Existing data is preserved; unused directories are not
removed automatically. Parent directories above the requested directory must exist.

Paths are absolute when the host can walk from the module image (native
separators). If that fails, engine game-dir fallback may still fill `game`.

---

## Join a leaf

```c
authum_path_t path;
authum_path_join(&path, authum_host_get_module_data_path(host), "cache.bin");
/* fopen(authum_path_get_cstr(&path), "rb") */
```

- Capacity: `authum_path_capacity` (see `<authum/path/capacity.h>`).
- Module joins insert `/` (`authum_path_sep`); that still works with `fopen` on
  Windows HLDS.
- `authum_path_get_cstr` never returns null (empty → `""`);
  `authum_path_get_cstr_or_null` returns null when empty.

---

## With config / logging

For files use `authum_host_join_config_path(host, "qproto.cfg", &path)`
(`modules/qproto/configs/qproto.cfg`; `whitelist.ini` the same way) and
`authum_host_join_data_path(host, "bans.db", &path)` (`modules/<module>/data/bans.db`). Full cfg workflow: [cfg.md](cfg.md). Logger sinks: [logging.md](logging.md).

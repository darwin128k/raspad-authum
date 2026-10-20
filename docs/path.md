# Paths (`authum_path_t`) and host env

Modules must not invent `char *data_dir` / `getcwd` for Authum roots. The host
fills `authum_env_t` with fixed-capacity `authum_path_t` directories; modules
join leaves onto a copy.

Headers: `<authum/path.h>`, `<authum/env.h>`, `<authum/host.h>`.

---

## Env roots

| Getter | Typical use |
|--------|-------------|
| `authum_host_env_game` | Game directory (e.g. `cstrike`) |
| `authum_host_env_addons` | `…/addons` |
| `authum_host_env_authum` | `…/addons/authum` |
| `authum_host_env_modules` | `…/addons/authum/modules` |
| `authum_host_env_module` | This module’s folder (`…/modules/qproto`) — cfg, logs, bans |
| `authum_host_env_models` / `_sprites` | Content trees when needed |

Always via `authum_host_*` — never `host->env->…`.

Paths are absolute when the host can walk from the module image (native
separators). If that fails, engine game-dir fallback may still fill `game`.

---

## Join a leaf

```c
authum_path_t path;
authum_path_set(&path, authum_host_env_module(host));
authum_path_join(&path, "mymod.cfg");
/* fopen(authum_path_as_cstr(&path), "r") or authum_cfg_open(&cfg, &path) */
```

- Capacity: `authum_path_capacity` (see `<authum/path/capacity.h>`).
- Module joins insert `/` (`authum_path_sep`); that still works with `fopen` on
  Windows HLDS.
- `authum_path_as_cstr` never returns null (empty → `""`);
  `authum_path_as_cstr_or_null` returns null when empty.

---

## With config / logging

Prefer `authum_host_env_module` + `authum_path_join` for `*.cfg` and module log
files. Full cfg workflow: [cfg.md](cfg.md). Logger sinks: [logging.md](logging.md).

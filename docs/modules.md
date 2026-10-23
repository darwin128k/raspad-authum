# Authum modules

What is implemented today: an Authum module is an `lh_os_module_t`, the
loader privilege comes from lh, and Authum only decides which files to
offer it.

Headers: `authum/module.h`, `authum/module/info.h`, `authum/loader.h`.
The mechanism underneath is documented in lh (`docs/module-loader.md`
there): image, `start` / `stop`, owner, loader, and the child list.

---

## 1. The model

A module is the lh module. Authum does not wrap it in a second struct.
`authum_module_t` is `lh_os_module_t`.

The module carries an image (`lh_os_shared_t`), its methods
(`lh_os_module_ops_t`), private state (`data`), and an owner (the loader
that loaded it; null for the host). Loading other modules is a privilege
the module is granted (`lh_os_module_grant_loader`). A module that was
never granted a loader is a leaf. The loader the privilege creates is an
`lh_os_loader_t`: it opens one image, reads the method table that image
exports, calls `start`, and links the module into a list. It does not
choose directories and it does not know whether a child will load
children of its own.

Authum's part is the policy around that mechanism:

- identity and ABI live in an exported `authum_module_info_t`;
- a module exports `authum_module_api` (`start` / `stop`); `authum_mm`
  itself is bound to lh through its own `authum_module_ops`;
- extra modules come from `configs/modules.list` when that file exists,
  otherwise from shared libraries in a directory named `modules` beside
  the image;
- the host plugin file `authum_mm` is never opened as a child.

One module does not call another directly. There is no `dlsym` into a
sibling and no shared function table; modules talk through the host's
event bus (forwards and queries by name, see [events.md](events.md)). A child can see its parent
(`lh_os_module_get_parent`) because the loader records the owner before
`start`. That is the whole link.

---

## 2. Two exports

Every image that wants to be an Authum module exports two C symbols.

| Symbol | Type | Role |
|--------|------|------|
| `authum_module_info` | `authum_module_info_t` | Who the module is, and which loader ABI it was built for. |
| `authum_module_api` | `authum_module_api_t` | `start(host)` and `stop`. An image without it is not a module and is not kept. |

`authum_module_info` fields:

| Field | Meaning |
|-------|---------|
| `abi` | Loader contract. Accepted when `authum_version_is_abi_compatible` against the host's `AUTHUM_MODULE_ABI` (`1.0.0`): same major (0.x: same minor). |
| `name` | Required. A null name is rejected. |
| `version` | The module's own SemVer. |
| `datetime` | Build timestamp. |
| `author`, `url`, `logtag` | Reported identity. Metamod's `plugin_info_t` for this host is filled from the same object. |

`start` may be `authum_module_start` and `stop` may be `authum_module_stop`.
Both are exported from the host image, so a child built against that
export can point its table at them. A child may supply its own functions
instead. lh calls whatever the child's table contains.

---

## 3. What `start` does

`authum_module_start` runs after the image is already open.

1. Read `authum_module_info`. Missing symbol, missing name, or an
   incompatible ABI returns false. The pointer is stored with
   `lh_os_module_set_data`. `authum_module_get_info` reads it back.
2. `lh_os_module_grant_loader(self, "authum_module_ops")`. The new loader
   is empty. Failure (already granted, or out of memory) returns false.
3. If this module has an owner, load children for its own image (see
   below). The host has no owner, so its `start` does not load children.
   `Meta_Attach` calls `authum_loader_load_modules` for the host after init.

A false return refuses the module. lh then destroys it and does not link
it. `authum_module_stop` only clears the identity pointer. Children are
already gone: lh unloads them before `stop`.

Refusal reasons Authum writes into the OS last-error slot:

| Code | Value | When |
|------|-------|------|
| `authum_error_code_missing_info` | `0x20` | `authum_module_info` is not exported. |
| `authum_error_code_invalid_info` | `0x21` | The object has no name. |
| `authum_error_code_incompatible_abi` | `0x22` | `abi` is not compatible with `0.1.0`. |
| `authum_error_code_bad_modules_list` | `0x24` | `modules.list` is present but not valid Authum list text. |

An open or filesystem failure stays in lh's own last-error
(`lh_os_last_error` or `lh_os_system_last_error`).

---

## 4. How a child joins the list

The list does not invent paths. Authum finds paths and hands them over
one at a time. `lh_os_loader_load` is what links a node.

`authum_loader_load_modules` takes the image path, drops the last
segment (`lh_fs_path_parent`), and looks for `configs/modules.list`. If it
exists, only that list is used. If it is missing,
the name `modules` is joined and the directory is scanned (one level).
A missing directory, or a path that is not a directory, succeeds with
nothing loaded. An empty path is an error.

### 4.1 `modules.list`

Authum’s own list dialect (not INI). Built on `lh_str_line_reader_t`:
one path per line. Comments are whole lines starting with `;` or `#`.
Blank lines are ignored. Optional sections filter by OS.

```
# comments ; or #
hello

[linux]
foo
bar

[win32]
foo
```

Lines before any section, and under `[modules]` / `[any]`, apply on every
OS. `[linux]` / `[win32]` only on that OS (Metamod keywords). Other
section names are ignored. Relative paths join under `modules/` beside
the image; absolute paths (`lh_fs_path_is_absolute`) are used as-is. The
host file `authum_mm` is rejected by `authum_loader_load` and not opened.

When the file is present, an empty matching set succeeds with nothing
loaded — the directory is not also scanned. A bad section header returns
false (`authum_error_code_bad_modules_list`).

### 4.2 Directory scan

For each entry in `modules/`:

- directories and symlinks are skipped;
- the name must end with the platform suffix (`.dll` / `.so`), case-insensitive;
- the host file `authum_mm` plus that suffix is skipped and not opened;
- anything else is passed to `authum_loader_load`.

`authum_loader_load` requires the loader privilege. It then calls
`lh_os_loader_load`, which:

1. Allocates a module (`lh_os_module_create`).
2. Sets its owner to this loader, so `start` can already see the parent.
3. Opens the image.
4. Reads `authum_module_api`. No symbol: the module is destroyed and not linked.
5. Calls `start`. False: destroyed and not linked. A child whose `start`
   is `authum_module_start` loads its own children here, into its own
   list.
6. On success, `lh_list_push_back` links the module's node at the tail.

One rejected file does not fail the list. A directory-read or list parse
error does: children already linked stay linked.

Order is load order. The tail is the last success. Unload walks backward
(`lh_os_loader_unload` pops the tail, and destroying a module unloads
that module's children first).

```c
for (authum_module_t *m = authum_loader_get_first(self); m != NULL;
     m = authum_loader_get_next(self, m))
{
    const authum_module_info_t *info = authum_module_get_info(m);
}
```

`authum_loader_get(self, index)` is the same list walked to that index.
`authum_loader_get_loaded` is the length.

---

## 5. Host lifetime

The host object is the global `authum`.

`authum_loader_init` initializes it, binds the image that contains
`authum_module_info` (`lh_os_module_bind`), sets `authum_module_ops`, and
starts it. `Meta_Attach` then calls `authum_loader_load_modules` and logs
each child's name and version, then `authum_gate_init` (connect gate —
see [gate.md](gate.md)).

`Meta_Detach` calls `authum_gate_deinit`, then `authum_loader_deinit`
(`lh_os_module_deinit`). Close stops first: children unload, then
`authum_module_stop`, then the image reference is released.

`authum_loader_unload_all` unloads children and leaves the module itself
loaded, so the same loader can be used again.

---

## 6. Layout beside an image

```
authum_mm.dll
configs/modules.list        # optional; if present, replaces the dir scan
modules/
    one.dll
    two.dll
    two.dll's own modules/  # loaded from two's start, if its start does that
```

`configs/` and `modules/` of the host are siblings of `authum_mm.dll`,
not children of the process working directory.

Example (see `deploy/configs/modules.list`):

```
hello
```

That is the module folder `modules/hello/` and its image `modules/hello/hello.so`
(`.dll` on Windows). Without `modules.list`, every folder `modules/<name>/` with
a `<name>.so` in it is loaded. Modules are
built by their own projects — Authum only loads them.

No `.so` / `.dll` needed — Authum appends the native shared suffix. Use
no section (or `[modules]` / `[any]`) for all OSes; `[linux]` / `[win32]`
still filter by platform when you want different lists.

On attach Authum logs every load attempt (`module OK:` / `module FAIL: …`)
into the server log. Console command `authum` reprints the loaded list
(no RCON `meta list` required for Authum children).

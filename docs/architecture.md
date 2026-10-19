# Architecture: host facade + optional private deps

Authum follows **option B with an escape hatch** (same idea as Metamod’s
one host, many plugins — without a shared `lh.so`).

## Why not a shared lh

lh is a **static** library on purpose (GCC + exported/PIC kills inlining
across translation units). Authum will not ship a process-wide `lh.so` for
modules to link. See lh’s own CMake notes / history (`d87f9eff`).

## Layers

| Layer | Who owns it | How many copies |
|-------|-------------|-----------------|
| Host services (log, engfuncs, globals, …) | `authum_mm` | **One** — call via parent symbols / [`authum/host.h`](../include/authum/host.h) |
| lh inside Authum | linked into `authum_mm` | One (host implementation detail) |
| Full lh / other libs in a module | that module’s `.so` | Optional, **private** — not the host runtime |

## Contract for module authors

1. Export `authum_module_info` + `authum_module_ops` (see [writing-a-module.md](writing-a-module.md)).
2. For **shared** behaviour use the host facade (`authum_host_bind`,
   `authum_host_log_line`, engfuncs/globals). Do **not** expect
   `lh_runtime_logger()` in the module to be `authum.log`.
3. **Escape hatch:** need string/FS helpers or another author’s library?
   Link them into *your* module. Own static lh from Authum’s `lib/lh`
   submodule is fine; it is a second copy and must not assume host globals.

## Example

[`examples/hello`](../examples/hello/) — bind host, log one line, no private lh calls beyond what the ABI headers need.

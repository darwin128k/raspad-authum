# Architecture: host facade + optional private deps

Authum follows **option B with an escape hatch** (Metamod-like one host —
without a shared `lh.so`).

## Why not a shared lh

lh is **static** on purpose (GCC + exported/PIC kills inlining). Authum does
not ship a process-wide `lh.so` for modules.

## Layers

| Layer | Who | Copies |
|-------|-----|--------|
| Host facade (`authum_host_t`: log, engfuncs, globals, `module_dir`, `crypto`, `engine`) | `authum_mm` | **One** — passed into `authum_module_api.start` |
| Crypto facade (`authum/crypto/…`, same shape as lh, subset only) | headers + ops in `authum_mm` | Modules call Authum names; lh stays inside the host |
| Engine facade (`authum/engine.h` — game dir / print / command) | ops in `authum_mm` | Modules need no HLSDK for the common subset |
| lh + HLSDK inside Authum | linked / includes in `authum_mm` | Implementation detail of the host |
| Full lh / HLSDK / other libs in a module | that module’s `.so` | Optional, **private** escape hatch |

## Client module contract (ABI 0.2)

Include **`authum/client.h` only** — no `<lh/…>`, no link to `liblh`.

Export:

1. `authum_module_info` — identity / ABI (`0.2.0`)
2. `authum_module_api` — `{ start(host), stop() }`

The host resolves these symbols, fills `authum_host_t`, calls `start`.

**Escape hatch:** link your own static lh (from Authum’s `lib/lh`) or any
other library into *your* `.so` for private code. That is not the host runtime;
`lh_runtime_logger()` there is not `authum.log`.

## Example

[`examples/hello`](../examples/hello/) — pure client module (no lh).

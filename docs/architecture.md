# Architecture: host facade + optional private deps

Authum follows **option B with an escape hatch** (Metamod-like one host —
without a shared `lh.so`).

The guiding rule: **a module contains only its own logic.** Talking to the
engine, owning memory past unload, parsing config, and talking to other
modules are Authum's job. The less code in a module, the cleaner its API.

## Why not a shared lh

lh is **static** on purpose (GCC + exported/PIC kills inlining). Authum does
not ship a process-wide `lh.so` for modules.

## Layers

| Layer | Who | Copies |
|-------|-----|--------|
| Host facade (`authum_host_t`: log, engfuncs, globals, `env`, `crypto`, `engine`, `sandbox`, `events`, `commands`) | `authum` | **One** per module — passed into `authum_module_api.start` |
| Client API (`authum_client` static lib: host getters, cfg, path, cstr, logger, sandbox, array, events) | `src/authum/api/*.c` | Linked into `authum` and into every module; headers only declare |
| Foundation types (`authum_byte_t`, `authum_usize_t`, `authum_bool_t`, …) | Authum headers | Same idea as lh types; modules never include lh |
| Sandbox (`authum/sandbox.h`) | ops in `authum` on `lh_memory_tree` | One root per module, freed after `stop` — [sandbox.md](sandbox.md) |
| Event bus (`authum/events.h`) | ops in `authum` | Forwards + queries between modules; host publishes `authum.client.*` — [events.md](events.md) |
| Commands (`authum/commands.h`) | registry in `authum` | One engine command `authum`; modules add `authum <module> <name>` — [commands.md](commands.md) |
| Crypto facade (`authum/crypto/…`, same shape as lh, subset only) | ops in `authum` | Modules call Authum names; lh stays inside the host |
| Engine facade (`authum/engine.h` — game dir / print / command / kick) | ops in `authum` | Modules need no HLSDK for the common subset |
| lh + HLSDK inside Authum | linked / includes in `authum` | Implementation detail of the host |
| Full lh / HLSDK / other libs in a module | that module’s `.so` | Optional, **private** escape hatch |

## Client module contract (ABI 1.0)

Include **`authum/client.h`**, link **`authum_client`** — no `<lh/…>`, no link
to `liblh`.

Export:

1. `authum_module_info` — identity / ABI (`1.0.0`)
2. `authum_module_api` — `{ start(host), stop() }`

The host resolves these symbols, fills `authum_host_t` (with a fresh sandbox
and an event handle bound to the module), calls `start`. After `stop` it drops
the module's subscriptions and frees its sandbox.

**Escape hatch:** link your own static lh (from Authum’s `lib/lh`) or any
other library into *your* `.so` for private code. That is not the host runtime;
`lh_runtime_logger()` there is not Authum's log.

## Tests

`tests/` (CTest, plain C, 32-bit like HLDS): `authum_client` helpers, the host
sandbox on `lh_memory_tree`, and the event bus (order, stop, queries,
re-entrancy, owner cleanup).

## Example

[`examples/hello`](../examples/hello/) — pure client module (no lh): host
facade, an `authum.client.connect` subscription, a sandbox array.

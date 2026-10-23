# Authum connect gate

Product version **1.0.0** adds a connect gate to the host. The loader stays
inside Authum; auth (and later 2D/3D) are **separate** modules dropped under
`modules/` — Authum does not ship or compile them.

## Idea

1. Client reaches HLDS Steam validation.
2. Authum’s hook runs first (`Steam_NotifyClientConnect` on classic HLDS).
3. Registered authorizers look at the ticket / context.
4. Allow → write AuthID and continue; reject → disconnect; nobody claims →
   native Steam path.

`ClientConnect` in the gamedll is too late for non-Steam tickets.

## Branching

| Branch | Role |
|--------|------|
| `0.1.0` | Freeze: loader + `modules.list` only |
| `1.0.0` | Gate host + `modules.list` / `authum.log` (auth module ships separately) |
| `2.0.0` | Mainline host work after 1.0.0 |

Auth modules (e.g. [raspad-qproto](https://github.com/darwin128k/raspad-qproto))
are built and versioned in their own repos.

## Module ABI

See `AUTHUM_MODULE_ABI` / [`writing-a-module.md`](writing-a-module.md). Host
fills `authum_host_t` and calls `authum_module_api.start`.

## Layout

```
addons/authum/
  authum.so
  modules.list          # one folder name per line
  modules/
    <name>/
      <name>.so         # drop-in from that module’s project
```

Console: `authum` lists modules. Server log shows `module OK` / `module FAIL`.

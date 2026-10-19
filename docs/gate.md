# Authum connect gate

Product version **1.0.0** adds a connect gate to the host. The loader stays
inside Authum; auth (and later 2D/3D) are modules under it.

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
| `master` | Mainline (merge from version branches when ready) |

Reference freeze of the standalone plugin: **qproto `1.0.0`** /
`v1.0.0` on [raspad-qproto](https://github.com/darwin128k/raspad-qproto).

## Module ABI

`AUTHUM_MODULE_ABI` stays `0.1.0` until authorizer registration is part of
the exported module contract. Until then the host may grow a C API that
children bind from the Authum image during `start`.

## Status

Host gate init is wired; **auth lives in the qproto Authum module**:

```
addons/authum/
  authum_mm.so
  modules.list                # qproto
  modules/qproto/
    qproto.so
    qproto.cfg
    qproto_bans.cfg
    qproto.log                # created at runtime
```

Console: `authum` lists modules. Server log shows `module OK` / `module FAIL`.

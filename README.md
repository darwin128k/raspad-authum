<p align="center">
  <img src="assets/logo.png" alt="Authum" width="280">
</p>

<h1 align="center">Authum</h1>

<p align="center">
  <b>2.0.0</b> — Metamod-P plugin host for classic HLDS<br>
  loads child modules from <code>modules.list</code> / <code>modules/</code>
</p>

<p align="center">
  <a href="LICENSE">Apache-2.0</a> ·
  <a href="CHANGELOG.md">Changelog</a> ·
  <a href="https://github.com/darwin128k/raspad-authum">GitHub</a>
</p>

---

## What it is

Metamod-P **host** for original (32-bit) HLDS. Builds only `authum_mm` — child
modules (e.g. [qproto](https://github.com/darwin128k/raspad-qproto)) are separate
projects dropped under `modules/`.

- Loads modules from `modules.list` (Authum line format) or scans `modules/`
- Connect gate scaffolding for children
- Client modules: `authum/client.h` (ABI **1.0**, no lh type in its headers);
  host passes `authum_host_t` into `start` (sandbox, events, commands)
- Server command `authum` — status / load / unload / module commands
- Static `lh` in the host (`AUTHUM_LH_LINK_MODE=needed|whole`); `authum_client`
  runs `<authum/json.h>` / `<authum/sql.h>` on it too, with lh's symbols kept
  local to each module (`--exclude-libs`)

## Branches

| Branch | Meaning |
|--------|---------|
| `master` | Mainline |
| `1.0.0` | Freeze — gate + host log + `modules.list` |
| `0.1.0` | Freeze — loader only |

## Install

Ready Linux binary: download `authum_mm.so` from
[Releases](https://github.com/darwin128k/raspad-authum/releases).

1. Put into `cstrike/addons/authum/`:
   - `authum_mm.so`
   - `configs/modules.list` (one module name per line) and `configs/authum.cfg`
   - `modules/<name>.so` (built by that module’s own project)
2. In `addons/metamod/plugins.ini`:

```text
linux addons/authum/authum_mm.so
```

3. `meta list` → `authum` RUN; console `authum` lists children

Layout:

```text
cstrike/addons/metamod/plugins.ini   →  linux addons/authum/authum_mm.so
cstrike/addons/authum/
  authum_mm.so
  modules/                           ← one .so per module: qproto.so, gatekeeper.so, …
  configs/                           ← everything the admin edits
    authum.cfg                       ← log levels
    modules.list                     ← which modules to load
    qproto.cfg  gatekeeper.cfg  …    ← one <module>.cfg per module
    gatekeeper/whitelist.ini         ← a module's other files
  data/                              ← what modules write themselves (databases, keys)
    gatekeeper/  qproto/  lang/
  logs/<authum|module>/YYYY-MM-DD.log ← written at runtime
```

Back up `configs/` and `data/`; the rest is replaced by an update.

## Docs

| Doc | |
|-----|--|
| [CHANGELOG.md](CHANGELOG.md) | Releases |
| [docs/architecture.md](docs/architecture.md) | Host facade + optional private deps |
| [examples/](examples/) | Module templates: skeleton, events, engine, chat filter, cvar check, config, SQLite, module-to-module bus |
| [docs/README.md](docs/README.md) | Index |
| [docs/writing-a-module.md](docs/writing-a-module.md) | Write your own module |
| [docs/modules.md](docs/modules.md) | Loader / `modules.list` |
| [docs/logging.md](docs/logging.md) | Logging policy (host vs module) |
| [docs/gate.md](docs/gate.md) | Connect gate |

## Clone

```sh
git clone --recurse-submodules https://github.com/darwin128k/raspad-authum.git
```

If you already cloned without submodules:

```sh
git submodule update --init --recursive
```

`lib/lh` is a **required** submodule on branch **`0.4.0`** of
[darwin128K/lh](https://github.com/darwin128K/lh). Not `main`. CMake refuses
any other `lib/lh/VERSION`. To build against another lh tree (one being
worked on next to Authum), pass `-DAUTHUM_LH_DIR=/path/to/lh`.

You still need Metamod-P (+ HLSDK headers) under `lib/metamod-p`
(see `AUTHUM_HLSDK_DIR` in `CMakeLists.txt`).

## Build

HLDS loads 32-bit modules only. `CMakePresets.json` picks the toolchain for the
host system:

```sh
# Linux (gcc-multilib / g++-multilib)
cmake --preset linux32
cmake --build --preset linux32
ctest --preset linux32

# Windows (MSYS2 C:/msys64/mingw32: pacman -S mingw-w64-i686-gcc mingw-w64-i686-ninja)
cmake --preset windows-mingw32
cmake --build --preset windows-mingw32
ctest --preset windows-mingw32
```

The build goes to `build-<preset>/`. Without presets, pass a 32-bit capable
compiler yourself (`cmake -S . -B build-linux32 -G Ninja -DCMAKE_BUILD_TYPE=Release`).
A compiler that cannot link with `-m32` (MSYS2 `mingw64`, Linux without
multilib) is refused at configure time, not at the first link.

Optional: `-DAUTHUM_LH_LINK_MODE=needed` (default) or `whole`.
`LH_STATIC_LIBGCC` is forced on so the module does not depend on the host’s
`libgcc_s` (HLDS).

Linux CI builds `authum_mm.so` on `master` only. Tag `v*` publishes the `.so`
as a release asset. Output: `build-linux32/authum_mm.so`.


## Acknowledgements / sources

Authum builds on (and vendors where noted) the following:

| Component | Role | License / upstream |
|-----------|------|--------------------|
| **Metamod-P** | Host plugin API (`lib/metamod-p`, CI clone) | [Bots-United/metamod-p](https://github.com/Bots-United/metamod-p) |
| **HLSDK** | Engine / game headers (via Metamod-P tree) | Valve HLSDK (as shipped with Metamod-P) |
| **lh** | Host internals; JSON and SQL behind `<authum/json.h>` / `<authum/sql.h>` (`lib/lh` submodule, branch `0.4.0`) | [darwin128K/lh](https://github.com/darwin128K/lh) |
| **SQLite** | 3.46.1 amalgamation for `<authum/sql.h>`, vendored in lh (`lib/lh/lib/sqlite`) | [sqlite.org](https://www.sqlite.org/) ([copyright](https://www.sqlite.org/copyright.html)) |
| **Monocypher** | Ed25519 signatures behind `<authum/crypto/ed25519.h>` (`lib/monocypher`, 4.0.2) | BSD-2-Clause / CC0 — [monocypher.org](https://monocypher.org/) |



See also [NOTICE](NOTICE).

## License

Apache License 2.0 — see [LICENSE](LICENSE) and [NOTICE](NOTICE).

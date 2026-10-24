<p align="center">
  <img src="assets/logo.png" alt="Authum" width="280">
</p>

<h1 align="center">Authum</h1>

<p align="center">
  <b>4.0.0</b> — Metamod-P plugin host for classic HLDS<br>
  loads child modules from <code>modules.list</code> / <code>modules/</code>
</p>

<p align="center">
  <a href="LICENSE">Apache-2.0</a> ·
  <a href="CHANGELOG.md">Changelog</a> ·
  <a href="https://github.com/darwin128k/raspad-authum">GitHub</a>
</p>

---

## What it is

Authum **as the HLDS game library** — the engine loads `authum.so` where the
game library used to be, and Authum loads the real game behind it. Builds only
`authum` — child modules (e.g.
[qproto](https://github.com/darwin128k/raspad-qproto)) are separate projects
dropped under `modules/`.

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
| `4.0.0` | The game-library line: Authum as the game library (no Metamod) |
| `3.0.0` | Freeze — the Metamod-P host |
| `2.0.1`, `2.0.0`, `1.0.0` | Earlier freezes |

## Install

Ready Linux binary: download `authum-linux32.zip` from
[Releases](https://github.com/darwin128k/raspad-authum/releases).

1. Unpack into the game dir (`cstrike/`) — you get `addons/authum/`:
   - `authum.so` (the game library)
   - `gamedll.txt` — the real game library, one line (`dlls/cs.so`)
   - `configs/modules.list` (one module name per line) and `configs/authum.cfg`
   - `modules/<name>/<name>.so` (built by that module’s own project)
2. In `liblist.gam`, point the engine at Authum:

```text
gamedll_linux "addons/authum/authum.so"
```

3. The server console shows the map loading; `authum` lists the children

Layout:

```text
cstrike/liblist.gam                 →  gamedll_linux "addons/authum/authum.so"
cstrike/addons/authum/
  authum.so                          ← the host image: the game library now
  gamedll.txt                        ← the real game library (dlls/cs.so)
  configs/                           ← Authum's: authum.cfg (log levels), modules.list
  logs/YYYY-MM-DD.log                ← Authum's log + every module's warnings and errors
  modules/                           ← one sandbox per module
    qproto/
      qproto.so
      configs/                       ← the admin's: qproto.cfg, …
      lang/                          ← translations
      data/                          ← what the module writes (databases, keys)
      logs/YYYY-MM-DD.log            ← the module's log
```

Install or remove a module = copy or delete its folder. An update replaces the
`.so` and `lang/`; it never touches `configs/`, `data/` or `logs/`.

## Standalone: Authum as the game library

The build *is* the game library: the engine loads `addons/authum/authum.so`
where the game used to be, and Authum loads the real game behind it. Same
layout for the admin's files — `addons/authum/` with `configs/`, `logs/`,
`modules/`.

```text
cstrike/addons/authum/authum.so      ← the host image (the game library now)
cstrike/addons/authum/gamedll.txt    ← the real game library: dlls/cs.so
cstrike/liblist.gam                  ←  gamedll_linux "addons/authum/authum.so"
```

Linux (i386) and Windows (i686) — same image, same layout. See
[docs/standalone.md](docs/standalone.md) for the engine's contract with the
game module and the build options.

## Docs

| Doc | |
|-----|--|
| [CHANGELOG.md](CHANGELOG.md) | Releases |
| [docs/architecture.md](docs/architecture.md) | Host facade + optional private deps |
| [docs/standalone.md](docs/standalone.md) | Authum as the game library (no Metamod) |
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

The HLSDK headers live in `lib/hlsdk` (vendored). Point the build at another
tree with `-DAUTHUM_HLSDK_DIR=/path/to/hlsdk`.

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

The game library the image wraps is named by file name — `cs.so` for
Counter-Strike (`-DAUTHUM_GAME_DLL_NAME=cs.so`); at run time it is found in
the game directory’s `dlls/`.

Linux CI builds `authum.so` on `master` and `4.0.0`. Tag `v*` publishes
`authum-linux32.zip` (the whole `addons/authum/` layout) as a release asset.
Output: `build-linux32/authum.so`.


## Acknowledgements / sources

Authum builds on (and vendors where noted) the following:

| Component | Role | License / upstream |
|-----------|------|--------------------|
| **HLSDK** | Engine / game headers (`lib/hlsdk`, vendored) | Valve HLSDK |
| **lh** | Host internals; JSON and SQL behind `<authum/json.h>` / `<authum/sql.h>` (`lib/lh` submodule, branch `0.4.0`) | [darwin128K/lh](https://github.com/darwin128K/lh) |
| **SQLite** | 3.46.1 amalgamation for `<authum/sql.h>`, vendored in lh (`lib/lh/lib/sqlite`) | [sqlite.org](https://www.sqlite.org/) ([copyright](https://www.sqlite.org/copyright.html)) |
| **Monocypher** | Ed25519 signatures behind `<authum/crypto/ed25519.h>` (`lib/monocypher`, 4.0.2) | BSD-2-Clause / CC0 — [monocypher.org](https://monocypher.org/) |



See also [NOTICE](NOTICE).

## License

Apache License 2.0 — see [LICENSE](LICENSE) and [NOTICE](NOTICE).

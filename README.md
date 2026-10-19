<p align="center">
  <img src="assets/logo.png" alt="Authum" width="280">
</p>

<h1 align="center">Authum</h1>

<p align="center">
  <b>1.0.0</b> — Metamod-P plugin host for classic HLDS<br>
  loads child modules from <code>modules.list</code> / <code>modules/</code>
</p>

<p align="center">
  <a href="LICENSE">Apache-2.0</a> ·
  <a href="CHANGELOG.md">Changelog</a> ·
  <a href="https://github.com/darwin128k/rapad-authum">GitHub</a>
</p>

---

## What it is

Metamod-P **host** for original (32-bit) HLDS. Builds only `authum_mm` — child
modules (e.g. [qproto](https://github.com/darwin128k/raspad-qproto)) are separate
projects dropped under `modules/`.

- Loads modules from `modules.list` (Authum line format) or scans `modules/`
- Connect gate scaffolding for children
- Host facade for modules (`authum/host.h`: log, engfuncs, globals) — one
  runtime in `authum_mm`; modules may still link private lh / other libs
- Server command `authum` — list loaded modules
- Static `lh` inside the host only (`AUTHUM_LH_LINK_MODE=needed|whole`)

## Branches

| Branch | Meaning |
|--------|---------|
| `master` | Mainline |
| `1.0.0` | Freeze — gate + host log + `modules.list` |
| `0.1.0` | Freeze — loader only |

## Install

Ready Linux binary: download `authum_mm.so` from
[Releases](https://github.com/darwin128k/rapad-authum/releases).

1. Put into `cstrike/addons/authum/`:
   - `authum_mm.so`
   - `modules.list` (one module path per line, e.g. `qproto`)
   - `modules/<name>/<name>.so` (or `.dll` on Windows)
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
  modules.list
  modules/<name>/
    <name>.so
  authum.log                         ← written at runtime
```

## Docs

| Doc | |
|-----|--|
| [CHANGELOG.md](CHANGELOG.md) | Releases |
| [docs/architecture.md](docs/architecture.md) | Host facade + optional private deps |
| [examples/](examples/) | `hello` module |
| [docs/README.md](docs/README.md) | Index |
| [docs/writing-a-module.md](docs/writing-a-module.md) | Write your own module |
| [docs/modules.md](docs/modules.md) | Loader / `modules.list` |
| [docs/logging.md](docs/logging.md) | Logging policy (host vs module) |
| [docs/gate.md](docs/gate.md) | Connect gate |

## Clone

```sh
git clone --recurse-submodules https://github.com/darwin128k/rapad-authum.git
```

If you already cloned without submodules:

```sh
git submodule update --init --recursive
```

`lib/lh` is a **required** submodule on branch **`0.4.0`** of
[darwin128K/lh](https://github.com/darwin128K/lh). Not `main`. CMake refuses
any other `lib/lh/VERSION`.

You still need Metamod-P (+ HLSDK headers) under `lib/metamod-p`
(see `AUTHUM_HLSDK_DIR` in `CMakeLists.txt`).

## Build

```sh
cmake -S . -B build-linux32 -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-linux32
```

Optional: `-DAUTHUM_LH_LINK_MODE=needed` (default) or `whole`.
`LH_STATIC_LIBGCC` is forced on so the module does not depend on the host’s
`libgcc_s` (HLDS).

Linux CI builds `authum_mm.so` on `master` only. Tag `v*` publishes the `.so`
as a release asset. Output: `build-linux32/authum_mm.so`.

## License

Apache License 2.0 — see [LICENSE](LICENSE) and [NOTICE](NOTICE).

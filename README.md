# Authum

Metamod-P plugin host for original (32-bit) HLDS. Loads child modules from
`modules.list` beside the plugin image (or scans `modules/` if the file is
missing). Authum builds **only** `authum_mm` — modules (e.g. qproto) are
separate projects dropped under `modules/`.

## Branches

| Branch | Meaning |
|--------|---------|
| `0.1.0` | Freeze — loader only |
| `1.0.0` | Connect gate + host log + `modules.list` |
| `master` | Mainline |

## Drop-in layout (HLDS)

```
cstrike/addons/metamod/plugins.ini   →  linux addons/authum/authum_mm.so
cstrike/addons/authum/
  authum_mm.so
  modules.list                       →  one path per line (e.g. qproto)
  modules/<name>/
    <name>.so                        ← Authum picks .so / .dll by OS
```

Build artifact: `build-linux32/authum_mm.so`.
Console: `authum` lists modules; load OK/FAIL goes to the server log
and `addons/authum/authum.log`.

## Docs

| Doc | |
|-----|--|
| [docs/README.md](docs/README.md) | Index |
| [docs/writing-a-module.md](docs/writing-a-module.md) | Write your own module |
| [docs/modules.md](docs/modules.md) | Loader / `modules.list` |
| [docs/logging.md](docs/logging.md) | Logging policy (host vs module) |
| [docs/gate.md](docs/gate.md) | Connect gate |

## License

Apache License 2.0 — use and redistribute freely. If you change the sources,
say so in the modified files and keep the author attribution (`LICENSE`,
`NOTICE`).

## Clone

```sh
git clone --recurse-submodules https://github.com/darwin128k/rapad-authum.git
```

If you already cloned without submodules:

```sh
git submodule update --init --recursive
```

`lib/lh` is a **required** submodule on branch **`0.4.0`** of
[darwin128K/lh](https://github.com/darwin128K/lh) (INI parser /
`LH_STATIC_LIBGCC`). Not `main`. CMake refuses any other `lib/lh/VERSION`.

You still need Half-Life SDK headers and Metamod-P under `lib/` (see
`AUTHUM_HLSDK_DIR` in `CMakeLists.txt`).

## Build (Linux 32-bit)

```sh
cmake -S . -B build-linux32 -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-linux32
```

Optional: `-DAUTHUM_LH_LINK_MODE=needed` (default) or `whole`.
`LH_STATIC_LIBGCC` is forced on so the module does not depend on the host’s
`libgcc_s` (HLDS).

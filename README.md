# Authum

Metamod-P plugin host for original (32-bit) HLDS. Loads child modules from
`modules.ini` beside the plugin image (or scans `modules/` if the file is
missing).

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

`lib/lh` is a submodule pinned to branch **`0.4.0`** of
[darwin128K/lh](https://github.com/darwin128K/lh). That is the INI parser /
`LH_STATIC_LIBGCC` line Authum builds against — not `main`.

You still need Half-Life SDK headers and Metamod-P under `lib/` (see
`AUTHUM_HLSDK_DIR` in `CMakeLists.txt`).

## Build (Linux 32-bit)

```sh
cmake -S . -B build-linux32 -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-linux32
```

`LH_STATIC_LIBGCC` is forced on so the module does not depend on the host’s
`libgcc_s` (HLDS).

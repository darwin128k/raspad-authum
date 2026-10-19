# Examples

Sample Authum **modules** (not Metamod plugins). See
[docs/architecture.md](../docs/architecture.md): host facade for shared
services; optional private lh / other libs in your own `.so`.

| Example | What it shows |
|---------|----------------|
| [hello](hello/) | `authum_host_bind` + `authum_host_log_line` only |

## Build

From the Authum repo root (submodules initialized):

```sh
cmake -S . -B build-linux32 -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-linux32 --target hello
```

Artifact: `build-linux32/examples/hello/hello.so` (or `hello.dll`).

## Install

```text
modules.list          ← line: hello
modules/hello/hello.so
```

Lines go to `addons/authum/authum.log` through the host.

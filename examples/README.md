# Examples

| Example | What it shows |
|---------|----------------|
| [hello](hello/) | Client ABI 0.2: `authum/client.h` only, **no liblh** |

See [docs/architecture.md](../docs/architecture.md).

## Build

```sh
cmake -S . -B build-linux32 -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-linux32 --target hello
```

Artifact: `build-linux32/examples/hello/hello.so`

## Install

```text
modules.list          ← hello
modules/hello/hello.so
```

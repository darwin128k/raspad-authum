# HLDS asynchronous primary-log research

**Experimental Linux/glibc i386 prototype. Not installed, not included in the
Authum build, and not ready for a production server.**

The original HLDS 10319 server keeps partial `LMMDDNNN.log` output in a stdio
buffer. The public game DLL `AlertMessage` callback cannot capture all native
records: the engine also writes log headers, server cvars and close records.

This experiment operates below that callback. In the supplied original
`filesystem_stdio.so`, `CBaseFileSystem::FPrintf` reaches
`CFileSystem_Stdio::FS_vfprintf`, which imports glibc `__vfprintf_chk`.
An explicit `LD_PRELOAD` library can intercept that imported call without
engine memory offsets, byte signatures, vtable edits or changing the engine
binary. This is ELF symbol interposition, not a supported Valve logging API.

## What the prototype does

- Watches only an explicitly configured, canonical absolute directory and
  write-opened filenames matching `L` + seven digits + `.log`.
- Leaves filename selection, file opening, log formatting, timestamps and
  console/network output with the engine.
- Copies complete formatted records into a preallocated 1024-record SPSC
  queue. Formatted writes do not allocate, wait, acquire a mutex or perform
  file I/O. The record buffer is 2048 bytes; oversized records are dropped
  whole rather than truncated.
- A dedicated thread writes the original bytes to the original FILE stream
  and flushes after a batch; idle polling sleeps 10ms. This is a polling
  interval, **not a deadline** when the disk or scheduler is slow.
- Drops newest records on overflow, counts losses, and reports totals on
  exit. It does not synchronously fall back to disk.
- Drains and joins on close before allowing the engine to close or reuse the
  FILE pointer. Explicit flush also waits. Worker code calls libc only,
  never the engine/filesystem interface.

Only one matching log stream and one producer thread are supported. Further
matching streams while one is active retain native behavior. Additional
producer threads on the tracked stream drop records. This is an experimental
contract, not a production concurrency design.

## Reproduce

On Linux with Python 3, GCC and i386 libc development libraries:

```sh
python3 tools/experiments/hlds-async-log/run.py
```

To additionally test the real original Linux Counter-Strike HLDS installation:

```sh
python3 tools/experiments/hlds-async-log/run.py --server /path/to/raspad-server
```

Add `--report-dir /path/to/new/report` to preserve the test summary and server
console/log files outside `/tmp` (particularly useful when WSL shuts down and
clears temporary files). The report directory must not already exist.

The runner creates an isolated temporary installation, neutralizes copied
configuration scripts and loads `dlls/cs.so` directly. It attempts an
intentionally nonexistent map to complete startup command processing; this
is **not an active-map integration test**. It does not load Authum or
Metamod, alter the source installation, or connect to the production server.
Assets and runtime libraries are linked read-only by convention; the game
DLLs and top-level game files are copied. The output directory is printed
and left for inspection until the OS clears temporary files. Only run against
an installation you trust.

The opt-in environment variables are `AUTHUM_ASYNC_LOG_PROBE_DIR` and
`AUTHUM_ASYNC_LOG_PROBE_STALL_MS`. The runner sets them only for its child
processes. Do not put this prototype in a system-wide preload configuration.

## Evidence

Checked on 2026-10-26, WSL Ubuntu-22.04, GCC `-m32 -O2 -Wall -Wextra -Werror`.

- Native stdio: the first short record remains invisible until buffer flush.
- Async: short records become visible before close; 100 records spanning
  4096 bytes match byte-for-byte; close/reopen does not leak old records;
  an unrelated filename keeps native buffering.
- With the worker delayed for 1000ms, 2048 producer calls took approximately
  2ms in this environment. Exactly 1024 complete records survived in FIFO
  order, and exactly 1024 losses were reported. This is a mechanism test,
  **not a game-frame latency benchmark**.
- Original HLDS 10319 without Authum: native log start is 0 bytes while open,
  162 after `log off`. With the prototype: 121 while open, 162 after close.
  The engine-generated start and close lines are present once each.

Reference engine SHA-256:
`c7b28c7af8e1fa8e3a34c45107b6496cca2ff6d8764aa1af20894f0cdd978312`.
Filesystem SHA-256:
`5d2f8a837de037fe57db26ab9aac8f36fa5c9be64e6115ca2406927fded4950a`.
These identify the tested binaries; the prototype does not use these hashes
or binary addresses to attach.

## What remains before a production implementation

Opening and closing files still wait on the main thread. This prototype
removes ordinary formatted disk writes from that thread; it does not make
the engine's entire log lifecycle asynchronous. The engine also performs
directory creation and probes existing filenames itself.

Investigate a separate opt-in Linux adapter with process lifetime, independent
of unloadable modules. A late-loaded Authum module cannot assume that its
`vfprintf` export will replace bindings the engine already resolved. A preload
adapter avoids that timing issue but changes the server launch configuration.

Required further checks/design:

1. Real active-map A/B tests with and without Authum, map transitions,
   repeated log on/off, cvar dumps, player events and graceful shutdown.
2. Main-thread latency measurements with real and stalled storage. Decide
   explicitly whether synchronous open/close at map transitions is acceptable.
3. Multiple streams/producers, coordinated flush/close, queue sizing and
   overload accounting that remains observable during a long-running server.
4. Disk full, permission loss, short/failed writes and deferred error reporting.
   This prototype merely counts write/flush errors; it cannot return those
   asynchronous errors to the original formatted-write caller.
5. Audit all operations on the tracked FILE stream. Writes through other
   libc entry points are not intercepted and could race/reorder. The test
   confirms the supplied filesystem's formatted-write path only.
6. Check supported glibc/HLDS builds and sanitizers. Windows and other libc
   implementations require a separate integration and are not supported here.

No `fsync` durability or crash-time drain is promised. Standard process abort,
SIGKILL and power loss may lose queued data. Production fallback and failure
policies must be explicit; queue overflow must never silently turn into a
blocking disk write.

Relevant upstream interfaces:
[Valve IFileSystem](https://github.com/ValveSoftware/halflife/blob/master/public/FileSystem.h),
[ReHLDS reference log implementation](https://github.com/rehlds/ReHLDS/blob/master/rehlds/engine/sv_log.cpp).
ReHLDS is a reference only; the actual binary tests used original HLDS 10319.

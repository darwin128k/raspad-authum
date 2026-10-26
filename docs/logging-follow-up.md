# Logging follow-up

Recorded 2026-10-26. Investigate before reorganizing Authum logging, per the
user's updated priority.

- [ ] Investigate delayed writes to the engine's primary HLDS log and resolve
  any Authum contribution. Agree on flushing behavior so a running server's
  log does not misleadingly appear truncated after copying it.

Observed on the production server:

- `C:\Users\Vega\Desktop\raspad-server\cstrike\logs\L1025000.log`
  and `C:\Users\Vega\Desktop\L1025001.log` were exactly 4096 bytes and ended
  partway through a timestamp (`L 10/25/`).
- The server remained running. After a map change, more log data appeared.
- `C:\Users\Vega\Desktop\L1025000.log` was 6789 bytes and ended with
  `Log file closed` at 2026-10-25 21:28:05.

## Investigation on 2026-10-26

- The supplied installation is original Valve HLDS, protocol 48,
  1.1.2.7/Stdio, build 10319 (Jan 24 2025), not ReHLDS.
- Disassembly of that installation's `engine_i486.so` shows that `Log_Printf`
  calls `FS_FPrintf` and returns without a subsequent `FS_Flush`. `Log_Open`
  closes the previous file through `FS_Close` before opening the next one.
- An isolated WSL launch of those binaries with the original `dlls/cs.so`
  loaded directly, without Authum or Metamod, reproduced delayed visibility:
  the console contained `Log file started`, while `logs/L1026000.log` stayed
  at 0 bytes. After `log off`, it became 162 bytes and contained both the
  start and close records. This confirms buffering without Authum; it does
  not reproduce a complete active-map session or prove that Authum has no
  other logging defects.
- Probe artifacts: WSL Ubuntu-22.04,
  `/tmp/authum-engine-log-xob1wr7n/console.txt` and
  `/tmp/authum-engine-log-u3ot3v1e/console.txt`. Temporary probe source:
  `build/probe_engine_logs.py`. Existing server files were not changed.

The observed 4096-byte partial production files are consistent with the
engine's buffering. Their partial final line alone is not evidence of a
crash. Authum's `engine_alert_message` hook in
`src/authum/gamedll/engine.cpp` formats and forwards messages but does not
own the engine's file or its buffer. The exported SDK engine table does not
provide access to that log file's handle or a log-file flush operation.

## Remaining decision and checks

This issue is diagnosed, **not fixed**. Do not silently add `fflush(NULL)`,
per-frame disk flushing, engine-global memory offsets, or calls to engine
APIs from the disk worker. Those do not provide a safe nonblocking fix.

The user chose to investigate asynchronous replacement of primary-log writes.
This is separate from Authum's module file writer and must capture
engine-generated records too (server cvars and log open/close records do not
necessarily pass through the game DLL's `AlertMessage` hook).

### Completed feasibility experiment

See [the isolated prototype and research](../tools/experiments/hlds-async-log/README.md).
This Linux/glibc i386 preload prototype intercepts formatted writes to an
explicitly selected log directory, queues owned record bytes, and writes and
flushes from its own thread. It uses no engine offsets or byte patches.

`run.py --server /mnt/c/Users/Vega/Desktop/raspad-server` passed on 2026-10-26.
Tests ran in WSL Ubuntu-22.04. A subsequent run with `--report-dir` saves
the summary and native/async server logs in `build/hlds-async-log-report/`.
Temporary WSL `/tmp` directories may be cleared when the distribution stops.

- Native and asynchronous driver tests checked full byte equality across
  4096 bytes, close/reopen and unrelated-file isolation.
- A 1000ms worker stall did not block producer writes: 2048 calls took
  approximately 2ms; 1024 records were retained in order and 1024 dropped.
- The real original HLDS process, without Authum, had 0 bytes visible while
  its log was open and 162 bytes after close. With the prototype the values
  were 121 and 162; both engine-generated start and close records survived.

This demonstrates feasibility, not a production fix. The isolated startup
probe did not run an active map. File open, explicit flush and close still
wait on the main thread. The prototype is not part of the Authum build and
was not deployed or added to any server launch configuration.

For a production implementation, use a separately managed opt-in adapter
with process lifetime rather than assuming that a late-loaded module can
interpose already-resolved libc calls. Address the remaining lifecycle,
failure and multi-producer limitations documented with the prototype first.

Before calling a future fix complete, verify live file visibility during an
active map, records below and across 4096 bytes, map-change flushing, normal
shutdown, queue overflow, and game-thread latency. Repeat with and without
Authum. Retain all original production logs as evidence.

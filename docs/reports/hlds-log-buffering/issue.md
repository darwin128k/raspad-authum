## Summary

On original Linux HLDS build 10319, an engine-generated log message can appear in the server console while the corresponding `cstrike/logs/LMMDDNNN.log` file remains empty. Closing logging with `log off` makes the buffered contents visible.

Please consider a configurable periodic flush policy for the primary server log. This report concerns visibility of log data during normal operation, not a confirmed crash, permanent truncation, or data loss after a graceful close.

## Environment

- Original Valve HLDS, Linux i386, Counter-Strike 1.6; not ReHLDS.
- Ubuntu 22.04.5 LTS under WSL2; glibc 2.35.
- Kernel: `6.18.33.2-microsoft-standard-WSL2`, x86_64 host.
- The native verification loaded `cstrike/dlls/cs.so` directly, with no preload library.
- Tested build, not a claim that this is the newest available build.

Version reported at startup:

```text
Protocol version 48
Exe version 1.1.2.7/Stdio (cstrike)
Exe build: 20:09:04 Jan 24 2025 (10319)
```

## Native reproduction and observations

In an isolated installation, configuration scripts were neutralized and the original engine and game DLL binaries were used. The following launch arguments were used:

```sh
LD_LIBRARY_PATH=.:${LD_LIBRARY_PATH} ./hlds_linux \
  -game cstrike -console -insecure -nomaster -port 27991 \
  +maxplayers 2 +sv_lan 1 +log on +map log_probe_no_such_map
```

The map name above is intentionally nonexistent: this was a startup/logging probe, **not a successful active-map test**. The actual probe used a different arbitrary nonexistent name. There were also `sound/materials.txt` stat warnings in the isolated WSL installation; this is not being presented as a fresh-install active-gameplay reproduction.

1. Send `log on` through the server console/stdin if logging has not opened yet.
2. Observe `Log file started` in the console.
3. Read the new `cstrike/logs/L*.log` from a separate reader after 200ms, while logging is still enabled.
4. Send `log off`, then read that same file again.

Observed native file size: **0 bytes while open; 162 bytes after `log off`**. The separate reader used ordinary file reads, not `fsync`, filesystem-cache dropping, or a read of the server's in-process stdio buffer. The native empty-while-open behavior was also observed with a wait of approximately two seconds in an earlier probe.

Console excerpt from the recorded native test:

```text
Server logging data to file logs/L1026000.log
L 10/26/2026 - 08:09:12: Log file started (file "logs/L1026000.log") (game "cstrike") (version "48/1.1.2.7/Stdio/10319")
L 10/26/2026 - 08:09:12: Log file closed
Server logging disabled.
```

Native file contents while open: **empty**.

Complete native file contents after `log off`:

```text
L 10/26/2026 - 08:09:12: Log file started (file "logs/L1026000.log") (game "cstrike") (version "48/1.1.2.7/Stdio/10319")
L 10/26/2026 - 08:09:12: Log file closed
```

## Separate experiment: asynchronous writes

As a feasibility experiment, a local Linux/glibc preload prototype intercepted formatted writes to the selected log file, queued owned bytes, and wrote/flushed them from a worker thread. This is a third-party experiment, **not a Valve fix or a native configuration option**. File open/close remained synchronous.

| Test | While the log was open | After `log off` |
| --- | ---: | ---: |
| Native engine/file-system behavior | 0 bytes | 162 bytes |
| Experimental asynchronous writer | 121 bytes | 162 bytes |

Complete experimental file contents while open:

```text
L 10/26/2026 - 08:09:18: Log file started (file "logs/L1026000.log") (game "cstrike") (version "48/1.1.2.7/Stdio/10319")
```

Complete experimental file contents after `log off`:

```text
L 10/26/2026 - 08:09:18: Log file started (file "logs/L1026000.log") (game "cstrike") (version "48/1.1.2.7/Stdio/10319")
L 10/26/2026 - 08:09:18: Log file closed
```

Timestamps differ because these were separate runs. Neither record was duplicated or lost in these small tests. The experiment does not establish production readiness, active-map compatibility, or game-frame latency guarantees.

## Operational impact and request

This was investigated after copies of a running production server's logs appeared to stop halfway through a timestamp at exactly 4096 bytes. The server was still running, and a later copy contained more data and a normal close record. That observation is consistent with buffering; it is not proof of a crash.

Would it be possible to expose a configurable periodic flush interval, or document an existing supported setting for bounded-delay log visibility? Ideally this would avoid forcing a blocking disk flush for every message in the game thread. No power-loss durability or `fsync` guarantee is being requested.

The confirmed native result here is delayed visibility of the short engine-generated log header until close. Active-map timing, broader platform coverage, and behavior of newer builds remain to be checked.

# Host disk logger

## Module policy and configuration

Each loaded module automatically receives its own log channel. The sandbox
layout stays `configs/`, `data/`, `logs/`; daily files live in the actual
module directory's `logs/YYYY-MM-DD.log`. Authum writes its own daily log in
`addons/authum/logs/`. No module-owned threads, file handles or formatter
function pointers are retained by the writer.

The shortest normal API is:

```c
#include <authum/logger.h>
authum_host_log_info(host, "loaded %u bans", count);
```

Existing `authum_logger_init` and `authum_logger_*` calls still work.

Configuration precedence, lowest to highest:

1. Built-in defaults: info source level, warning console/host copies,
   file on, standard format, retention 10 days.
2. Unprefixed `log_*` defaults in `addons/authum/configs/authum.cfg`.
3. Module `configs/<module>.cfg`, unprefixed `log_*` keys, then
   `<journal>.log_*` keys for a named journal.
4. Explicit `<module>.log_*` keys in the host config, then
   `<module>.<journal>.log_*` keys.
5. Optional process-wide `AUTHUM_LOG_LEVEL`, sampled once at logger startup.
   It overrides source levels only. An invalid value does not override.

Missing and invalid values inherit the previous valid setting. Configuration
files are read when channels register and on map reload, not per message.
A module-wide `log_level=off` also disables its journals, even if a journal
requests a more verbose level. Ordinary module level thresholds otherwise
can be overridden by more specific journal settings.

| Key | Meaning |
| --- | --- |
| `log_level` | Source filter: emergency through debug; `off`/`none` disables every destination |
| `log_file` | `on`/`off` for this channel's own sandbox file only |
| `log_console` | Independent console threshold, or `off` |
| `log_host` | Independent threshold for copies into Authum's log, or `off` |
| `log_format` | `standard` or `raw`, applied to this channel's own file |
| `log_keep_days` | Calendar-day retention, 0 keeps everything; valid range 0..36500 |

Console and host copies must also pass the source filter. Host copies use
the host file's format, retention and source threshold. Enabling/disabling
a private file never changes console routing implicitly.

For example, `qproto/configs/qproto.cfg`:

```ini
log_level = debug
log_keep_days = 3
log_console = warning
log_host = off
bans.log_format = raw
```

An administrator can still disable it in `configs/authum.cfg` with
`qproto.log_level = off`.

## Standard line format

Standard file records use the HLDS timestamp prefix, followed by the level and text:

```text
L 10/26/2026 - 08:09:18: [info] loaded 3 bans
```

The timestamp is local time captured when the message is submitted. A module's
own file already identifies the module by its directory, so its name is omitted.
A copy in Authum's shared log includes the originating module or module/journal:

```text
L 10/26/2026 - 08:09:18: [warning] [qproto] ticket rejected
L 10/26/2026 - 08:09:18: [warning] [qproto/audit] ticket rejected
```

Authum's own records omit a redundant `authum` tag. Raw journals retain their
text-only format. Daily filenames remain `YYYY-MM-DD.log`.

The tag is the module's own mark: `authum_module_info_t.tag` — what the module
says it is called in the records. An empty tag falls back to the module's name.
A journal under a module is `<module tag>/<journal name>`. The name stays what
the channel is called: the folder under the module and the admin keys in the
configuration (`qproto.`, `qproto/bans.`).

## Special loggers

Open a named journal with `authum_logger_open`, or use the constructor
to specify a built-in formatter as a default:

```c
authum_logger_t audit;
authum_logger_options_t options = {
    sizeof options, "audit", authum_logger_format_raw
};
if (authum_logger_create(&audit, host, &options)) {
    authum_logger_info(&audit, "player=42 action=kick");
    authum_logger_close(&audit);
}
```

This writes under the same sandbox's `logs/audit/`. `raw` omits host
prefixes and preserves a terminating newline, adding one if absent.
Local and explicit administrator format settings override construction defaults;
global format defaults apply when construction uses `inherit`. Opening an already-shared
name with conflicting construction defaults fails.
Arbitrary plugin callbacks/custom output paths are deliberately not part
of this constructor: a queued record must stay valid after module unload.

## Retention

Cleanup registers with the disk worker when a channel opens or settings
reload. It runs then and every 24 hours, even if the channel is idle or
its messages are disabled. Registration shares the bounded queue; overflow
is counted with other dropped records and the next reload retries it.
The worker removes only valid `YYYY-MM-DD.log` dates strictly older than
the channel's cutoff. Unrecognized names, invalid dates, directories and
files currently open in the writer's cache are kept. Retention does not
limit the total size of a noisy current-day log.
The worker retains at most 1024 directory policies; additional directories
are cleaned on registration/reload only, with a worker-side diagnostic.

## Native HLDS log

These module settings do not change HLDS's `cstrike/logs/L*.log` backend.
The Linux preload research under `tools/experiments/hlds-async-log` is still
an isolated prototype, not loaded automatically by Authum. Do not infer that
the engine's native file open/close or console APIs are now asynchronous.

The public logger ABI, channel filtering, daily filenames, line format,
module journals and console routing described below use a dedicated
Authum-managed disk thread, with the configuration precedence and the complete
source-disable semantics documented above.

## Components

- host/logs.c: owner-thread registry, configuration, filtering, console callbacks,
  timestamp capture, the game server's line format and record submission.
- LH `lh/os/logger/queue`: the bounded SPSC queue and its worker thread, which
  sleeps on a condition until a record, a sync or its 100 ms flush period.
- LH `lh/os/logger/daily`: worker-only channels, directories, daily rotation by the
  local day of each line, retention and appending writes.

A channel is opened and closed by records in the same queue as its lines, under a
numeric id: the worker learns its directory, retention and format before its first
line, and nothing on the worker needs a lock. The 1024-slot queue owns each record
(channel id, tag, text, event time, level). It never retains plugin pointers. Closing a journal or
unloading a module cannot invalidate pending records. Calendar filenames and
line timestamps derive from event time, even if the writer crosses midnight.
Text capacity is 1023 bytes, matching the SDK's 1024-byte formatting buffer.

Submit allocates nothing, takes no locks and does no I/O. Full queues drop
newest; the worker reports loss in the host log without recursively queueing
that report. The fixed file cache holds at most 32 streams and evicts the least
recently used stream with fclose. Reopening appends. Buffers flush every 100ms
while storage responds, independently of server frames. Idle queue checks sleep
10ms, so producers need no wake syscall. Error records flush on the worker.
An unavailable directory or failed file operation produces a first-error stderr
diagnostic including the path; recovery after startup is best effort.

Shutdown is owner-thread only: stop accepting records, request stop, drain,
flush/close files, then join. It can wait for storage. A hard crash or forced
termination can lose records. fflush provides visibility to readers, not fsync
power-loss durability. No synchronous fallback writes exist for a full queue.

## Thread ownership

Registry lifecycle, writes, configuration and console callbacks remain on the
host/game thread. Use existing logger buffers for worker capture and deliver
on task completion. This change does not make direct worker access to host
logger ops safe. Formatting and engine console callbacks are synchronous;
configuration reads still occur at startup, channel registration and map reload.

host/logs.h exposes a private authum_logs_sync(timeout_ms) barrier for tests or
explicit administrative operations. Never use it in a frame callback. A barrier
waits for earlier accepted records and flush completion, not fsync or proof of
successful disk writes. Production frame hooks do not wait for logging.

## Checks

CTest authum_logs verifies routing, filters, journals, date rollover, retention,
independent periodic flushing, 40-journal cache eviction/reopen and shutdown
persistence. authum_log_writer deliberately blocks the sink, fills the queue,
checks nonblocking overflow and its report, owned copies, FIFO, barrier timeout,
ring reuse, shutdown drain and restart. The same queue test runs under ASan/UBSan
in FastDL's native sanitizer build.

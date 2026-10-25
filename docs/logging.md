# Host disk logger

The public logger ABI, channel filtering, daily filenames, line format,
module journals and console routing are unchanged. Since Authum 4.2.1, file
operations execute on a dedicated Authum-managed thread.

## Components

- host/logs.c: owner-thread registry, configuration, filtering, console callbacks,
  timestamp capture and owned record submission.
- host/logs/writer.c: bounded SPSC queue and worker lifecycle through Authum threads.
- host/logs/files.c: worker-only LRU cache, directories, daily rotation, retention,
  stdio buffering, flushing and file close.

The 1024-slot queue owns each record (path, tag, text, event time, level and
retention setting). It never retains plugin pointers. Closing a journal or
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

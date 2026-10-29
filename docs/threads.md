# Managed threads

`authum_host_threads(host)` returns a host-owned threading service. The SDK
contains only table-dispatch wrappers; the OS implementation lives in authum.so.
A host without threads (an unsupported platform) returns NULL; check it before
relying on the service.

Use `authum_thread_start(service, run, wake, user, cpu, flags, error, size)` for
a long-lived worker. `cpu = -1` inherits affinity. A configured CPU applies to
the new worker only. `AUTHUM_THREAD_BLOCK_SIGPIPE` blocks SIGPIPE in that worker
on POSIX without changing process-wide handlers or the game-thread signal mask.

Start and stop are owner/game-thread operations. `run(user)` executes on the
worker and must not call game-thread-only services. `wake(user)` runs on the
owner/game thread: signal an event or write a nonblocking pipe so run returns.
It must be nonblocking and must not call thread-management APIs. The context
and wake descriptors must remain valid until the host has joined the worker.

Explicit `authum_thread_stop(service, handle)` requests stop, joins and releases
the handle. Set your handle to NULL afterwards. Completed threads still need
this call (or owner cleanup) to release their OS resources.

On module unload the host closes the owner to new threads, calls every wake
callback first, then joins every worker, BEFORE calling module stop. This avoids
waiting on a worker whose shutdown depends on another worker's wakeup. The
module's stop callback may then free its worker contexts and release handles;
stopping an already joined owner handle is supported. Remaining records are
reclaimed after module stop. Failed module start follows owner cleanup too.

There are at most 64 outstanding handles per owner. A different owner's service
cannot stop a handle. No running worker is force-cancelled: shutdown depends on
cooperative wakeup and bounded I/O. Avoid waiting for game-frame callbacks while
stopping. This is lifecycle management inside one process, not isolation from
memory corruption or a hung filesystem.

The existing task queue remains for finite jobs and game-frame completions.
The thread service is for continuous loops, listeners and other workers with
an explicit wake/stop protocol. Both services remain owned by the module.

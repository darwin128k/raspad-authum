# Shared services

Modules keep their rules and schema; Authum supplies the reusable mechanics.
These APIs are in `authum_client`, with no lh/HLSDK types or new runtime libraries.

Native thread creation, affinity, mutexes, condition variables, sleep and monotonic
time are provided by LH's OS layer. Authum retains module ownership, queue limits,
main-frame completion delivery and cooperative shutdown before unloading code.
The public Authum thread/task/clock API and module ABI are unchanged.

## Worker logging

`<authum/logger/buffer.h>` provides a bounded log buffer. Ordinary logger calls
can be captured on a worker without calling the host's logger or its level
filter on that thread. Filtering and routing happen when the game thread flushes
the buffer; the original logger/journal is retained for each message.

1. Zero-initialize a buffer and call `authum_logger_buffer_init(&buffer, &log, 24)`
   when creating a request on the game thread.
2. Inside work, save `previous = authum_logger_buffer_begin(&buffer)`, use normal
   `authum_logger_info/error/...` calls, then `authum_logger_buffer_end(previous)`
   on every return path. Nested scopes restore their predecessor.
3. In completion, call `authum_logger_buffer_flush(&buffer)`.
4. In the request destructor, call `authum_logger_buffer_destroy(&buffer)`, even
   for cancelled requests that never reach completion.

The buffer accepts 1..256 lines, reports dropped messages, and never allocates
or calls the host while recording a line. A buffer has one owner at a time;
handoff uses the task queue. Logger handles must stay alive and not be rebound
until flush/discard. Flush only on the game thread. This does not make unrelated
host/engine/event/sandbox APIs safe to call from workers.

The capture buffer copies the channel binding for each record, so the
automatic-channel `authum_host_log_*` helpers also work inside a capture scope
without retaining their temporary logger handles. The underlying channel ops,
owner and any journal must still remain valid until flush/discard.

## References to a client connection

`<authum/client/ref.h>` provides `authum_clients_t` and value-type
`authum_client_ref_t`. This is a reference to one connection, not just a slot or
SteamID. References contain the tracker identity, its lifetime, generation,
index and userid.

Initialize a zeroed tracker with `authum_clients_init(&clients, host, 32)` before
registering your own player handlers. Existing connected players are recognized;
connect/put-in-server/disconnect and map activate/deactivate events subsequently
update the tracker. In a game-thread player handler:

```c
if (!authum_clients_capture(&clients, index, &request->client))
    return; /* disconnected, missing userid or invalid slot */
/* Copy request data to a worker; do not query the tracker there. */
```

Before applying the result on a game frame:

```c
if (!authum_client_ref_is_current(&clients, &request->client))
    return;
/* Safe to address that connection now; check again after external callbacks. */
```

A reconnect with the same userid still invalidates an old reference. Another
tracker cannot accidentally validate it. Destroy with `authum_clients_destroy`
before module stop, after background jobs have been drained; references are only
valid during that module/tracker lifetime. Calls are game-thread only and cannot
prevent a later disconnect after validation.

## SQL failure history

`authum_sql_errmsg` remains the last-operation message. The new
`authum_sql_first_error` retains the first failure on an open handle even when a
later statement succeeds. `authum_sql_clear_errors` explicitly acknowledges
handled failures. Reopening resets the history; connection-owned history ends
on close. Failed opens remain available through the last-operation message.

Threaded SQL automatically returns an unsuccessful result when its work left
an unacknowledged first error, even if the worker accidentally returned true.
The result contains that original message. A deliberately handled fallback must
clear the error explicitly. This lets modules remove their own sticky error
buffers and prepare/step wrappers. Schema, parameter values and the decision
whether to allow a player on a database error remain module policy.

## Network address parsing

`authum_net_address_host(address, out, capacity)` extracts a host without doing
DNS: IPv4/hostname with a port, bracketed IPv6 with a port, or a bare host/IPv6.
It supports in-place extraction. Invalid structure, invalid ports and output
truncation return false with empty output. It parses address structure; it does
not validate that a hostname or IP actually exists.

## Boundary of the extraction

Gatekeeper still owns IPHub JSON fields, the geo/proxy and ASN rules, whitelist
flags, its SQL tables and queries, and its resolved event. General worker/log,
connection-reference, SQL-error and address handling belong here.

The remaining reusable candidate is HTTP transport. Extract it separately with
tests for partial sends, response bounds, content length/chunking, deadlines and
the TLS/dependency contract; do not turn IPHub-specific parsing into a generic
network API merely by renaming it.

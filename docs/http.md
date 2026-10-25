# HTTP service and monotonic clock (ABI 1.2)

The host provides reusable, owner-bound HTTP servers. Modules describe resource
trees and policy; they do not implement sockets, request parsing, transfer
accounting, worker lifecycle or filesystem traversal.

    #include <authum/client.h>

    authum_http_options_t options;
    authum_http_options_init(&options);
    options.port = 17015;
    options.max_clients = 64;
    authum_cstr_copy(options.bind, sizeof options.bind, "0.0.0.0");
    authum_cstr_copy(options.allow_dirs, sizeof options.allow_dirs, "maps models sound");
    authum_cstr_copy(options.allow_extensions, sizeof options.allow_extensions, "bsp mdl wav");
    authum_http_source_add(&options, authum_path_as_cstr(authum_host_env_game(host)));

    authum_char_t error[512];
    authum_http_server_t *server =
        authum_http_open(authum_host_http(host), &options, error, sizeof error);
    /* Check server before proceeding; error describes startup failure. */

Check the HTTP facade and required environment paths when supporting older
hosts. Source paths must be absolute. Modules expand their own placeholders
before calling authum_http_source_add; check its return value for invalid paths
or overflow. Options are copied on open, so caller storage may be released.

The constructor starts with loopback binding and bounded resource limits.
Directory and extension policy is empty by default: explicitly choose allowed
extensions before opening. root_extensions separately controls file extensions
allowed directly at a tree root. FastDL sets it to wad; other plugins choose
their own policy. Sources, deny prefixes, size limits and symlink protection
apply equally to every plugin.

GET/HEAD static representations are supported. This is not a REST router and
does not run arbitrary module callbacks in the HTTP worker. No TLS, compression,
directory listing or request bodies. Linux transport is implemented; other
platforms report an explicit unsupported error.

URL names use ASCII letters, digits, underscores, hyphens, dots, slashes and
dollar signs. Escapes, query strings, hidden components and symlink traversal
are refused. Lists are case-sensitive. Responses use application/octet-stream;
Range requests receive the complete representation. These deliberate limits
keep the current static-resource service small and explicit.

## Lifecycle and logs

Call open/shutdown/close/drain on the owner/game thread. Shutdown joins the
worker but retains queued access records:

    authum_http_shutdown(service, server);
    authum_http_drain(service, server, write_access_log, user, 257);
    authum_http_close(service, server);
    server = NULL;

The log callback executes synchronously on the caller's thread. A bounded SPSC
queue holds 256 records and reports overflow. The module chooses drain budgets
and logging destination. The host stops owner workers before module stop and
reclaims forgotten servers, including failed starts. An owner's facade cannot
inspect, close or drain another owner's server. Up to eight servers per owner,
256 clients per server. Buffers use the configured client count.

A log callback may close its current server. Destruction is deferred until the
active drain call returns, and no further records are delivered to that callback.
Do not unload the owner/module while its callbacks are executing. Individual
drain calls are capped at 257 records even if a larger limit is requested.

## Monotonic clock

    const authum_clock_t *clock = authum_host_clock(host);
    authum_i64_t now_ms = authum_clock_monotonic_ms(clock);

Elapsed monotonic time is for deadlines, budgets and durations, not UTC or
game/map time. Worker reads are supported; keep the facade only while the host
is alive. Zero means unavailable. Linux uses CLOCK_MONOTONIC, Windows uses
QueryPerformanceCounter. Frame-based game timers remain a separate feature.

## Implementation map

The host/http directory contains:

- service.c: constructor validation, ownership and cleanup.
- server.c: resources and managed-thread lifecycle.
- loop.c: event dispatch and fair scheduling.
- transport.c: Linux socket/sendfile boundary.
- request.c and response.c: parsing and response headers.
- resources.c: file policy and safe descriptor-relative opening.
- limits.c: peer limits, deadlines and transfer budgets.
- access.c: sanitized, bounded access records.

Application state and public boundaries use Authum types. Native types remain
at real OS/libc/parser boundaries (descriptors, off_t, poll and size_t). The SDK
only dispatches host tables; HTTP implementation and picohttpparser are linked
once into authum.so.

Tests cover constructors, invalid options, owner isolation, clock behavior,
real responses, fragmented requests, 100 invalid-path cases, limits, partial
transfers, resets/SIGPIPE, access-log overflow and shutdown with active clients.
ASan/UBSan and formatting checks are available. These are not a production
security audit or provider-level DDoS filtering; memory/network remain shared.

#ifndef AUTHUM_HOST_HTTP_INTERNAL_H
#define AUTHUM_HOST_HTTP_INTERNAL_H

#include <authum/null.h>
#include <authum/host/http.h>
#include <authum/net.h>
#include <authum/host/clock.h>
#ifdef __cplusplus
#    include <atomic>
#else
#    include <stdatomic.h>
#endif
#include <lh/os/wake.h>
#include <lh/rail/loop.h>
#include <lh/os/net/socket.h>
#include <lh/os/fs/file.h>
#include <lh/core/http/forwarded.h>
#include <lh/core/http/limit/bucket.h>
#include <lh/core/http/limit/peer.h>
#include <authum/unused.h>

enum
{
    AUTHUM_HTTP_IO_ERROR = -1,
    AUTHUM_HTTP_IO_PENDING = -2,
    AUTHUM_HTTP_MAX_CLIENTS = 256,
    AUTHUM_HTTP_MAX_PEERS = 1024,
    AUTHUM_HTTP_LOG_CAPACITY = 256,
    AUTHUM_HTTP_LOG_LINE_CAPACITY = 768,
    AUTHUM_HTTP_REQUEST_CAPACITY = 8192,
    AUTHUM_HTTP_RESPONSE_CAPACITY = 512,
    /** Longest wait for a complete request head. */
    AUTHUM_HTTP_HEADER_TIMEOUT_MS = 5000,
    /** A transfer older than this must average at least the minimum rate. */
    AUTHUM_HTTP_RATE_GRACE_MS = 300000,
    AUTHUM_HTTP_MIN_BYTES_PER_SECOND = 4096,
    /** How long a reader with a partial request waits before its next read. */
    AUTHUM_HTTP_MIN_STEP_US = 2000,
    /** Partial reads of one request read at once, never deferred: a request
     *  sent in a few segments is not slowed, whatever the timer resolution. */
    AUTHUM_HTTP_FREE_PARTIALS = 4,
    /** Trusted proxy entries (addresses or CIDR blocks). */
    AUTHUM_HTTP_MAX_TRUSTED = 16,
    /** With every slot taken, the oldest unfinished request at least this old
     *  makes room for a new connection. */
    AUTHUM_HTTP_EVICT_MIN_AGE_MS = 250,
    /** Refused-request lines logged per second; the rest are only counted. */
    AUTHUM_HTTP_REFUSED_LINES_PER_SECOND = 20,
    /** Longest wait of the owner thread for the worker on stop. */
    AUTHUM_HTTP_STOP_TIMEOUT_MS = 1000
};

typedef struct authum_http_orphan authum_http_orphan_t;

/** authum_http_options_t as modules built before struct_size existed pass it.
 *  Never change: it is what those binaries put in memory. */
typedef struct authum_http_options_v0
{
    authum_char_t bind[64];
    authum_i32_t port;
    authum_char_t sources[AUTHUM_HTTP_MAX_SOURCES][AUTHUM_HTTP_PATH_CAPACITY];
    authum_usize_t source_count;
    authum_char_t allow_dirs[512];
    authum_char_t allow_extensions[512];
    authum_char_t root_extensions[512];
    authum_char_t deny_prefixes[512];
    authum_i32_t max_clients;
    authum_i32_t per_ip;
    authum_i32_t requests_per_second;
    authum_i32_t timeout_seconds;
    authum_i32_t cpu;
    authum_bool_t access_log;
    authum_u64_t bytes_per_second;
    authum_u64_t max_file_bytes;
} authum_http_options_v0_t;

/** The smallest sized struct: struct_size plus every field of the old layout. */
#define AUTHUM_HTTP_OPTIONS_MIN_SIZE (__builtin_offsetof(authum_http_options_t, trusted_proxies))

typedef enum authum_http_client_state
{
    authum_http_client_reading,
    authum_http_client_writing
} authum_http_client_state_t;

typedef struct authum_http_client
{
    lh_os_net_socket_t fd;
    lh_os_fs_file_t file;
    authum_i32_t ip_slot;
    authum_http_client_state_t state;
    authum_char_t input[AUTHUM_HTTP_REQUEST_CAPACITY];
    authum_char_t output[AUTHUM_HTTP_RESPONSE_CAPACITY];
    authum_usize_t used;
    authum_usize_t sent;
    authum_usize_t output_size;
    lh_u64_t offset;
    lh_u64_t remaining;
    authum_i64_t born;
    authum_i64_t progress;
    /** When a read last left the request incomplete (lh_os_tick_us). */
    lh_u64_t partial_us;
    /** Reads that left the request incomplete. */
    authum_u32_t partials;
    authum_u32_t id;
    authum_u32_t source;
    authum_i32_t status;
    authum_u64_t body_bytes;
    authum_char_t ip[AUTHUM_NET_IPV4_TEXT_SIZE];
    authum_char_t method[16];
    authum_char_t path[320];
    authum_u32_t port;
    const authum_char_t *close_reason;
    /** Why the request was refused (access log), or null. */
    const authum_char_t *reason;
    /** The peer's address key (network byte order). */
    authum_u32_t address;
    /** Counted in its peer's unfinished requests. */
    authum_bool_t pending;
    /** The peer is a trusted proxy: the client comes from X-Forwarded-For. */
    authum_bool_t proxied;
    /** The proxy the request came through, or empty. */
    authum_char_t via[AUTHUM_NET_IPV4_TEXT_SIZE];
} authum_http_client_t;

/** Refusals of the current second, logged as one summary line. */
typedef struct authum_http_refusals
{
    authum_i64_t second;
    authum_u32_t limit;
    authum_u32_t pending;
    authum_u32_t slots;
    authum_u32_t evicted;
    authum_u32_t logged;
    authum_u32_t unlogged;
} authum_http_refusals_t;

/** A worker loop the owner gave up on (lh_rail_loop_try_abandon), waiting to be
 *  reaped on the game thread. Allocated with the server, so that an abandon
 *  never needs memory. */
struct authum_http_orphan
{
    lh_rail_loop_t *loop;
    authum_http_orphan_t *next;
};

struct authum_http_server
{
    authum_http_server_t *next;
    authum_http_options_t options;
    /** The module that owns the server; it names the owner in the access log. */
    const authum_char_t *tag;
    authum_char_t tag_text[32];
    /** The trusted proxies, parsed from options.trusted_proxies. */
    lh_net_cidr_t trusted[AUTHUM_HTTP_MAX_TRUSTED];
    authum_u32_t trusted_count;
    authum_http_refusals_t refusals;
    /** Set by a stop that gave up waiting: the worker frees the server when it
     *  is done unless the owner drops it last (see authum_http_server_destroy). */
    authum_bool_t abandoned;
    /** The node the loop is reaped through if the stop gives up on it. */
    authum_http_orphan_t *orphan;
    lh_os_fs_file_t roots[AUTHUM_HTTP_MAX_SOURCES];
    lh_os_net_socket_t listener;
    lh_os_wake_t wake;
    /** The worker: a loop that sleeps in poll on the sockets and the wake signal. */
    lh_rail_loop_t *loop;
    /** What the next poll watches, and what the last one found ready. */
    lh_os_net_socket_poll_t events[AUTHUM_HTTP_MAX_CLIENTS + 2];
    authum_i64_t last_refill;
    /** Readers the next poll leaves out because their request is still arriving. */
    authum_u32_t deferred_readers;
    /** Microseconds until the first deferred reader is due. */
    lh_u64_t deferred_due_us;
    double tokens;
    authum_usize_t cursor;
    authum_http_client_t *clients;
    /** Per-address limits; the key is the address key (network byte order). */
    lh_http_limit_peer_t peers[AUTHUM_HTTP_MAX_PEERS];
    authum_u32_t next_id;
    authum_bool_t joined;
    authum_usize_t drain_depth;
    authum_bool_t close_pending;
#ifdef __cplusplus
    std::atomic_uint log_read;
    std::atomic_uint log_write;
    std::atomic_uint log_dropped;
    std::atomic_uint refs;
#else
    atomic_uint log_read;
    atomic_uint log_write;
    atomic_uint log_dropped;
    /** Holders of the server: the owner and the worker. */
    atomic_uint refs;
#endif
    authum_char_t logs[AUTHUM_HTTP_LOG_CAPACITY][AUTHUM_HTTP_LOG_LINE_CAPACITY];
};

AUTHUM_EXTERN_C_BEGIN

/**
 * @brief Return the file owned by an HTTP client for modification.
 *
 * @param client Initialized client, borrowed.
 * @return The initialized file, borrowed until the client is reset or released.
 */
lh_os_fs_file_t *
authum_http_client_get_file_mut(authum_http_client_t *client);

/**
 * @brief Return the server's initialized source-root array for modification.
 *
 * @param server Initialized server, borrowed.
 * @return The borrowed array of ::AUTHUM_HTTP_MAX_SOURCES roots; it does not
 *         outlive the server.
 */
lh_os_fs_file_t *
authum_http_server_get_roots_mut(authum_http_server_t *server);

/**
 * @brief Return the server's client array for modification.
 *
 * @param server Initialized server, borrowed.
 * @return The borrowed client array, or null before allocation; the array does
 *         not outlive the server.
 */
authum_http_client_t *
authum_http_server_get_clients_mut(authum_http_server_t *server);

authum_i64_t
authum_http_now_ms(void);

/** Release call of an abandoned worker loop: drops the worker's hold on @p arg. */
void
authum_http_worker_release(authum_ptr arg);

void
authum_http_access_record(authum_http_server_t *s, const authum_http_client_t *c,
                          const authum_char_t *event);
void
authum_http_access_token(authum_char_t *dst, authum_usize_t capacity, const authum_char_t *src,
                         authum_usize_t length);
void
authum_http_client_close(authum_http_server_t *s, authum_http_client_t *c);

/** Decode a request target once (%XX, query cut) and apply the publishing
 *  policy. Writes the relative path to @p out and returns its length, or 0
 *  with @p reason naming the refusal. */
authum_usize_t
authum_http_resource_check(const authum_http_options_t *options, const authum_char_t *url,
                           authum_usize_t length, authum_char_t *out, authum_usize_t capacity,
                           const authum_char_t **reason);
/** Open a checked relative path below @p root. On failure @p reason says why. */
authum_bool_t
authum_http_resource_open(lh_os_fs_file_t *file, const lh_os_fs_file_t *root,
                          const authum_http_options_t *options, const authum_char_t *path,
                          lh_u64_t *size, const authum_char_t **reason);
void
authum_http_response_set(authum_http_client_t *c, authum_i32_t code, const authum_char_t *reason,
                         lh_u64_t size, authum_i32_t head);
void
authum_http_request_read(authum_http_server_t *s, authum_http_client_t *c, authum_i64_t now);

authum_i32_t
authum_http_peer_acquire(authum_http_server_t *s, authum_u32_t ip, authum_i64_t now,
                         const authum_http_options_t *options);
/** Undo one acquire: the connection of @p slot is gone. */
void
authum_http_peer_release(authum_http_server_t *s, authum_i32_t slot);

/** Parse options->trusted_proxies; false with @p error on a bad entry. */
authum_bool_t
authum_http_trusted_parse(authum_http_server_t *s, authum_char_t *error, authum_u32_t error_size);

/** Whether an address key (network byte order) belongs to a trusted proxy. */
authum_bool_t
authum_http_trusted(const authum_http_server_t *s, authum_u32_t address);

/** The client of a trusted proxy's request: the rightmost untrusted
 *  X-Forwarded-For hop, or the proxy itself. */
authum_u32_t
authum_http_forwarded_client(const authum_http_server_t *s, const lh_str_view_t *const *values,
                             authum_usize_t count, authum_u32_t proxy);
/** Count a refused connection in this second's summary (kind: limit, pending, slots, evicted). */
void
authum_http_refusal_count(authum_http_server_t *s, authum_u32_t *counter, authum_i64_t now);

/** Write the previous second's refusal summary once the second has passed. */
void
authum_http_refusals_flush(authum_http_server_t *s, authum_i64_t now);

/** The end-of-connection access line, subject to the log level and the
 *  per-second budget for refused requests. */
void
authum_http_access_close(authum_http_server_t *s, const authum_http_client_t *c,
                         const authum_char_t *event, authum_i64_t now);
/** Read a caller's options in whatever layout it was built with (old layout,
 *  smaller or larger sized struct) into the current one, with host defaults for
 *  missing fields. False when the struct cannot be read. */
/** Frees what a server holds except its loop and its orphan node. */
void
authum_http_server_release_resources(authum_http_server_t *server);

authum_bool_t
authum_http_options_read(const authum_http_options_t *in, authum_http_options_t *out);

/** Queue one preformatted line for the owner's drain (dropped and counted when full). */
void
authum_http_access_line(authum_http_server_t *s, const authum_char_t *line);

/** Forget the last OS error, so a later message names only a new one. */
void
authum_http_error_reset(void);

/** Test-only: called at the start of every worker step when set. */
extern void (*authum_http_test_step_hook)(authum_http_server_t *server);

authum_bool_t
authum_http_client_expired(const authum_http_client_t *c, const authum_http_options_t *options,
                           authum_i64_t now);
void
authum_http_budget_refill(double *tokens, authum_i64_t *last, authum_i64_t now,
                          const authum_http_options_t *options);
/** One round of the worker: the sockets the last poll found ready, then the next poll's. */
void
authum_http_step(authum_ptr arg);

/** The worker's sleep: poll the sockets and the wake signal. */
void
authum_http_wait(authum_ptr arg, lh_u32_t timeout_ms);

/** Cut the worker's sleep short: signal its wake socket. */
void
authum_http_wake(authum_ptr arg);

/** What the worker's first poll watches: the listener and the wake signal. */
void
authum_http_prepare_first_poll(authum_http_server_t *server);

void
authum_http_server_release(authum_http_server_t *server);

authum_http_server_t *
authum_http_server_create(const authum_http_options_t *options, const authum_char_t *tag,
                          authum_char_t *error, authum_u32_t error_size);
void
authum_http_server_shutdown(authum_http_server_t *server);

void
authum_http_server_destroy(authum_http_server_t *server);

authum_usize_t
authum_http_server_memory(const authum_http_server_t *server);

authum_u32_t
authum_http_server_drain(authum_http_server_t *server, authum_http_log_fn sink, authum_ptr user,
                         authum_u32_t limit);
authum_bool_t
authum_http_listener_open(lh_os_net_socket_t *listener, const authum_http_options_t *options,
                          authum_char_t *error, authum_u32_t error_size);

/** Accept one connection into @p peer's socket (left empty when none waits). */
authum_bool_t
authum_http_socket_accept(lh_os_net_socket_t *listener, authum_http_client_t *peer,
                          authum_u32_t *address);

authum_i64_t
authum_http_socket_receive(lh_os_net_socket_t *socket, authum_char_t *buffer, authum_usize_t size);

authum_i64_t
authum_http_socket_send(lh_os_net_socket_t *socket, const authum_char_t *buffer,
                        authum_usize_t size);

authum_i64_t
authum_http_socket_send_file(lh_os_net_socket_t *socket, const lh_os_fs_file_t *file,
                             lh_u64_t *offset, authum_usize_t size);
/** The last OS error as text, for an error message. */
const authum_char_t *
authum_http_get_error_text(void);

void
authum_http_clients_accept(authum_http_server_t *server, authum_i64_t now);

void
authum_http_client_read(authum_http_server_t *server, authum_http_client_t *client,
                        authum_i64_t now);
void
authum_http_client_write(authum_http_server_t *server, authum_http_client_t *client,
                         authum_i64_t now, double *tokens);

/**
 * @brief With every slot taken, the connection that has waited longest without finishing its
 * request makes room, if it is old enough not to be a client whose request is simply still on the
 * way.
 */
authum_i32_t
evict_oldest_reader(authum_http_server_t *server, authum_i64_t now);

/**
 * @brief Note that @p client sent part of a request: when, and how many times.
 */
void
authum_http_client_partial(authum_http_client_t *client);

/**
 * @brief An address key (octets in network order) as a host-order number.
 */
authum_u32_t
authum_http_host_order(authum_u32_t key);

/**
 * @brief Write the address of key @p key (network byte order) into @p out.
 */
void
authum_http_key_to_ip4(authum_u32_t key, lh_net_ip4_t *out);

/**
 * @brief The address key (network byte order) of @p address.
 */
authum_u32_t
authum_http_key_of_ip4(const lh_net_ip4_t *address);

/**
 * @brief Fill @p events with what the next poll watches: the listener, the wake signal and each client, closing the timed-out ones.
 */
void
authum_http_prepare_connections(authum_http_server_t *server, lh_os_net_socket_poll_t *events,
                                authum_i64_t now, double tokens);

/**
 * @brief Act on what poll found for @p client: read, write within the byte budget, or close.
 */
void
authum_http_dispatch_connection(authum_http_server_t *server, authum_http_client_t *client,
                                lh_uint_t events, authum_i64_t now, double *tokens);

/**
 * @brief Answer @p client with error @p code and close after it.
 */
void
authum_http_request_refuse(authum_http_client_t *client, authum_i32_t code,
                           const authum_char_t *status, const authum_char_t *reason);

/**
 * @brief Content-Length is accepted only as an empty body: digits that are all zero.
 */
authum_bool_t
authum_http_request_is_zero_length(const lh_str_view_t *value);

/**
 * @brief Write the IPv4 address @p address (network order) as text, NUL-terminated.
 */
void
authum_http_request_format_address(authum_u32_t address, authum_char_t *text,
                                   authum_usize_t capacity);

/**
 * @brief Whether the space-separated @p list holds @p value (or a prefix of it, with @p prefix), without case.
 */
authum_bool_t
authum_http_policy_contains(const authum_char_t *list, lh_str_view_t value, authum_bool_t prefix);

/**
 * @brief Whether @p server belongs to @p owner.
 */
authum_bool_t
authum_http_owner_has_server(const authum_http_owner_t *owner, const authum_http_server_t *server);

/**
 * @brief Whether @p string has a NUL within its @p capacity.
 */
authum_bool_t
authum_http_is_terminated(const authum_char_t *string, authum_usize_t capacity);

/**
 * @brief Defaults for fields a caller's struct may predate; keep in step with
 * authum_http_options_init.
 */
void
authum_http_options_fill_defaults(authum_http_options_t *options);

/**
 * @brief Whether @p options are in bounds and their text fields terminated.
 */
authum_bool_t
authum_http_options_is_valid(const authum_http_options_t *options);

/**
 * @brief Open a server for module @p context: the service table's open.
 */
authum_http_server_t *
authum_http_service_open(authum_ptr context, const authum_http_options_t *caller,
                         authum_char_t *error, authum_usize_t error_size);

/**
 * @brief Open a server for @p owner from options already copied and checked.
 */
authum_http_server_t *
authum_http_service_open_read(authum_http_owner_t *owner, const authum_http_options_t *options,
                              authum_char_t *error, authum_usize_t error_size);

/**
 * @brief Stop @p server's worker, keeping its state: the service table's shutdown.
 */
void
authum_http_service_shutdown(authum_ptr context, authum_http_server_t *server);

/**
 * @brief Stop and free @p server, or mark it when a drain is running: the service table's close.
 */
void
authum_http_service_close(authum_ptr context, authum_http_server_t *server);

/**
 * @brief Hand at most @p limit access-log lines of @p server to @p sink: the service table's drain.
 */
authum_usize_t
authum_http_service_drain(authum_ptr owner, authum_http_server_t *server, authum_http_log_fn sink,
                          authum_ptr user, authum_usize_t limit);

/**
 * @brief Bytes @p server takes: the service table's memory usage.
 */
authum_usize_t
authum_http_service_get_memory(authum_ptr owner, const authum_http_server_t *server);

AUTHUM_EXTERN_C_END

#endif

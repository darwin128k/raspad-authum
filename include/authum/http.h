#ifndef AUTHUM_HTTP_H
#define AUTHUM_HTTP_H

#include <authum/size.h>
#include <authum/bool.h>
#include <authum/char.h>
#include <authum/extern.h>
#include <authum/ptr.h>

enum
{
    AUTHUM_HTTP_MAX_SOURCES = 16,
    AUTHUM_HTTP_PATH_CAPACITY = 4096
};

/**
 * Options for one HTTP server. Fill them with ::authum_http_options_init, which
 * also sets @c struct_size: the host reads only that many bytes and uses its
 * defaults for fields a smaller (older) struct does not have. A struct from
 * before @c struct_size existed (it starts with the bind text) is read in its
 * old layout, so modules built against it keep working.
 */
typedef struct authum_http_options
{
    /** sizeof the caller's struct; set by ::authum_http_options_init. */
    authum_u32_t struct_size;
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
    /** 0 off; 1 one line per request plus a per-second summary of refusals;
     *  2 also connect and request-start lines. */
    authum_bool_t access_log;
    authum_u64_t bytes_per_second;
    authum_u64_t max_file_bytes;
    /** Space- or comma-separated IPv4 addresses or CIDR blocks of trusted
     *  reverse proxies; empty trusts none. Only for a trusted peer is the
     *  client taken from X-Forwarded-For (the rightmost untrusted hop). */
    authum_char_t trusted_proxies[512];
    /** Unfinished requests (connected, request not yet complete) one address
     *  may hold at once. */
    authum_i32_t per_ip_pending;
    /* New fields go here, at the end; the host defaults them for smaller structs. */
} authum_http_options_t;

typedef struct authum_http_server authum_http_server_t;

AUTHUM_EXTERN_C_BEGIN

typedef void (*authum_http_log_fn)(const authum_char_t *line, authum_ptr user);

AUTHUM_EXTERN_C_END

typedef struct authum_http_ops
{
    authum_http_server_t *(*open)(authum_ptr owner, const authum_http_options_t *options,
                                  authum_char_t *error, authum_usize_t error_size);
    void (*shutdown)(authum_ptr owner, authum_http_server_t *server);
    void (*close)(authum_ptr owner, authum_http_server_t *server);
    authum_usize_t (*drain)(authum_ptr owner, authum_http_server_t *server, authum_http_log_fn sink,
                            authum_ptr user, authum_usize_t limit);
    authum_usize_t (*memory_usage)(authum_ptr owner, const authum_http_server_t *server);
} authum_http_ops_t;

typedef struct authum_http_service
{
    const authum_http_ops_t *ops;
    authum_ptr owner;
} authum_http_service_t;

AUTHUM_EXTERN_C_BEGIN

/** Static HTTP server constructor. Sources must be absolute resource-tree
 * paths; placeholder expansion belongs to the module. Configure before open.
 * GET/HEAD only; this is not a REST router. All API calls below are owner-thread
 * operations. The worker never invokes a module's callbacks. */
void
authum_http_options_init(authum_http_options_t *options);

authum_bool_t
authum_http_source_add(authum_http_options_t *options, const authum_char_t *path);

authum_http_server_t *
authum_http_open(const authum_http_service_t *service, const authum_http_options_t *options,
                 authum_char_t *error, authum_usize_t error_size);
/** Stop/join but retain queued access records until close. */
void
authum_http_shutdown(const authum_http_service_t *service, authum_http_server_t *server);

/** Stop/join and free the server. The host also reclaims forgotten servers.
 * Inside a drain callback, destruction waits until drain returns. */
void
authum_http_close(const authum_http_service_t *service, authum_http_server_t *server);

authum_usize_t
authum_http_drain(const authum_http_service_t *service, authum_http_server_t *server,
                  authum_http_log_fn sink, authum_ptr user, authum_usize_t limit);
authum_usize_t
authum_http_get_memory_usage(const authum_http_service_t *service,
                             const authum_http_server_t *server);

AUTHUM_EXTERN_C_END

#endif

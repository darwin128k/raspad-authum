#ifndef AUTHUM_HOST_HTTP_INTERNAL_H
#define AUTHUM_HOST_HTTP_INTERNAL_H

#include <authum/host/http.h>
#include <lh/os/system/net/socket.h>
#include <authum/net.h>
#include <authum/host/clock.h>
#ifdef __cplusplus
#    include <atomic>
#else
#    include <stdatomic.h>
#endif
#include <lh/os/wake.h>
#include <lh/os/system/fs/file.h>
#include <lh/str/format/text.h>
#include <lh/util/str/ptr.h>
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
    AUTHUM_HTTP_RESPONSE_CAPACITY = 512
};

typedef enum authum_http_client_state
{
    authum_http_client_reading,
    authum_http_client_writing
} authum_http_client_state_t;

typedef struct authum_http_client
{
    lh_os_system_net_socket_handle_t fd;
    lh_os_system_fs_file_handle_t file;
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
    authum_u32_t id;
    authum_u32_t source;
    authum_i32_t status;
    authum_u64_t body_bytes;
    authum_char_t ip[AUTHUM_NET_IPV4_TEXT_SIZE];
    authum_char_t method[16];
    authum_char_t path[320];
    authum_u32_t port;
    const authum_char_t *close_reason;
} authum_http_client_t;

typedef struct authum_http_peer
{
    authum_u32_t ip;
    authum_bool_t used;
    authum_u32_t active;
    authum_u32_t count;
    authum_i64_t window;
} authum_http_peer_t;

struct authum_http_server
{
    authum_http_server_t *next;
    authum_http_options_t options;
    lh_os_system_fs_file_handle_t roots[AUTHUM_HTTP_MAX_SOURCES];
    lh_os_system_net_socket_handle_t listener;
    lh_os_wake_t wake;
    authum_threads_t threads;
    authum_thread_t *thread;
    authum_http_client_t *clients;
    authum_http_peer_t peers[AUTHUM_HTTP_MAX_PEERS];
    authum_u32_t next_id;
    authum_bool_t joined;
    authum_usize_t drain_depth;
    authum_bool_t close_pending;
#ifdef __cplusplus
    std::atomic_uint log_read;
    std::atomic_uint log_write;
    std::atomic_uint log_dropped;
#else
    atomic_uint log_read;
    atomic_uint log_write;
    atomic_uint log_dropped;
#endif
    authum_char_t logs[AUTHUM_HTTP_LOG_CAPACITY][AUTHUM_HTTP_LOG_LINE_CAPACITY];
};

AUTHUM_EXTERN_C_BEGIN

authum_i64_t
authum_http_now_ms(void);

void
authum_http_access_record(authum_http_server_t *s, const authum_http_client_t *c,
                          const authum_char_t *event);
void
authum_http_access_token(authum_char_t *dst, authum_usize_t capacity, const authum_char_t *src,
                         authum_usize_t length);
void
authum_http_client_close(authum_http_server_t *s, authum_http_client_t *c);

lh_os_system_fs_file_handle_t
authum_http_resource_open(lh_os_system_fs_file_handle_t root, const authum_http_options_t *options,
                          const authum_char_t *url, authum_usize_t length, lh_u64_t *size);
void
authum_http_response_set(authum_http_client_t *c, authum_i32_t code, const authum_char_t *reason,
                         lh_u64_t size, authum_i32_t head);
void
authum_http_request_read(authum_http_client_t *c, const lh_os_system_fs_file_handle_t *roots,
                         const authum_http_options_t *options);
authum_i32_t
authum_http_peer_acquire(authum_http_server_t *s, authum_u32_t ip, authum_i64_t now,
                         const authum_http_options_t *options);
authum_bool_t
authum_http_client_expired(const authum_http_client_t *c, const authum_http_options_t *options,
                           authum_i64_t now);
void
authum_http_budget_refill(double *tokens, authum_i64_t *last, authum_i64_t now,
                          const authum_http_options_t *options);
void
authum_http_run(authum_ptr arg);

void
authum_http_wake(authum_ptr arg);

void
authum_http_server_release(authum_http_server_t *server);

authum_http_server_t *
authum_http_server_create(const authum_http_options_t *options, const authum_threads_t *threads,
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
lh_os_system_net_socket_handle_t
authum_http_listener_open(const authum_http_options_t *options, authum_char_t *error,
                          authum_u32_t error_size);
lh_os_system_net_socket_handle_t
authum_http_socket_accept(lh_os_system_net_socket_handle_t listener, authum_http_client_t *peer, authum_u32_t *address);

authum_i64_t
authum_http_socket_receive(lh_os_system_net_socket_handle_t fd, authum_char_t *buffer, authum_usize_t size);

authum_i64_t
authum_http_socket_send(lh_os_system_net_socket_handle_t fd, const authum_char_t *buffer, authum_usize_t size);

authum_i64_t
authum_http_socket_send_file(lh_os_system_net_socket_handle_t socket, lh_os_system_fs_file_handle_t file, lh_u64_t *offset,
                             authum_usize_t size);
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

AUTHUM_EXTERN_C_END

#endif

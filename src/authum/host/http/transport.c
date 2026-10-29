#define _GNU_SOURCE
#include <authum/host/http/internal.h>
#include <authum/unused.h>
#include <lh/char/map.h>
#include <lh/memory/std.h>
#include <lh/os/system.h>

authum_i64_t
authum_http_now_ms(void)
{
    return authum_clock_get_monotonic_ms(authum_clock_host());
}

authum_i32_t
authum_http_listener_open(const authum_http_options_t *options, authum_char_t *error,
                          authum_u32_t error_size)
{
    lh_net_ip4_t ip;
    lh_net_ip4_socket_addr_t address;
    if (!lh_net_ip4_parse(options->bind, strlen(options->bind), lh_addr_of(ip)))
    {
        snprintf(error, error_size, "bind '%s' must be an IPv4 address", options->bind);
        return -1;
    }
    lh_net_ip4_socket_addr_init(lh_addr_of(address), lh_addr_of(ip),
                                lh_cast_static(lh_net_port_t, options->port));
    authum_i32_t listener = lh_cast_static(
        authum_i32_t, lh_os_system_net_socket_open_nonblocking(lh_os_system_net_socket_type_tcp));
    if (lh_math_lt(listener, 0))
    {
        snprintf(error, error_size, "HTTP socket: %s",
                 strerror(lh_cast_static(authum_i32_t, lh_os_system_get_last_error_code())));
        return -1;
    }
    authum_unused(lh_os_system_net_socket_set_reuse_addr(listener, lh_bool_true));
    if (!lh_os_system_net_socket_bind(listener, lh_addr_of(address)) ||
        !lh_os_system_net_socket_listen(listener, 64))
    {
        snprintf(error, error_size, "cannot listen on %s:%d: %s", options->bind, options->port,
                 strerror(lh_cast_static(authum_i32_t, lh_os_system_get_last_error_code())));
        lh_os_system_net_socket_close(listener);
        return -1;
    }
    return listener;
}

authum_i32_t
authum_http_socket_accept(authum_i32_t listener, authum_http_client_t *peer, authum_u32_t *address)
{
    lh_net_ip4_socket_addr_t remote;
    lh_net_ip4_t ip;
    authum_i32_t socket = lh_cast_static(
        authum_i32_t, lh_os_system_net_socket_accept_nonblocking(listener, lh_addr_of(remote)));
    if (lh_math_lt(socket, 0))
    {
        return -1;
    }
    lh_net_ip4_socket_addr_get_ip(lh_addr_of(remote), lh_addr_of(ip));
    /* Keep the existing network-order byte key used by the per-peer limits. */
    lh_memory_std_copy(address, ip.octets, sizeof lh_ptr_deref(address));
    peer->port = lh_net_ip4_socket_addr_get_port(lh_addr_of(remote));
    lh_usize_t size = lh_net_ip4_format(lh_addr_of(ip), peer->ip, lh_math_sub(sizeof peer->ip, 1));
    peer->ip[size] = lh_char_map_nul;
    return socket;
}

authum_i64_t
authum_http_socket_receive(authum_i32_t fd, authum_char_t *buffer, authum_usize_t size)
{
    return lh_os_system_net_socket_recv(fd, buffer, size);
}

authum_i64_t
authum_http_socket_send(authum_i32_t fd, const authum_char_t *buffer, authum_usize_t size)
{
    return lh_os_system_net_socket_send(fd, buffer, size);
}

authum_i64_t
authum_http_socket_send_file(authum_i32_t socket, authum_i32_t file, off_t *offset,
                             authum_usize_t size)
{
    if (lh_math_lt(lh_ptr_deref(offset), 0))
    {
        return AUTHUM_HTTP_IO_ERROR;
    }
    lh_u64_t next = lh_cast_static(lh_u64_t, lh_ptr_deref(offset));
    lh_ssize_t result = lh_os_system_net_socket_send_file(socket, file, lh_addr_of(next), size);
    lh_ptr_deref(offset) = lh_cast_static(off_t, next);
    return result;
}

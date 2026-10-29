#include <authum/host/http/internal.h>
#include <lh/char/map.h>
#include <lh/memory/std.h>
#include <lh/os.h>
#include <authum/null.h>

authum_i64_t
authum_http_now_ms(void)
{
    return authum_clock_get_monotonic_ms(authum_clock_host());
}

lh_os_system_net_socket_handle_t
authum_http_listener_open(const authum_http_options_t *options, authum_char_t *error,
                          authum_u32_t error_size)
{
    lh_net_ip4_t ip;
    lh_net_ip4_socket_addr_t address;
    if (!lh_net_ip4_parse(options->bind, lh_str_ptr_len(options->bind), lh_addr_of(ip)))
    {
        authum_unused(lh_str_ptr_format_cstr(error, error_size, "bind '%s' must be an IPv4 address",
                                             options->bind));
        return LH_OS_SYSTEM_NET_SOCKET_HANDLE_INVALID;
    }
    lh_net_ip4_socket_addr_init(lh_addr_of(address), lh_addr_of(ip),
                                lh_cast_static(lh_net_port_t, options->port));
    lh_os_system_net_socket_handle_t listener =
        lh_os_system_net_socket_open_nonblocking(lh_os_system_net_socket_type_tcp);
    if (lh_math_eq(listener, LH_OS_SYSTEM_NET_SOCKET_HANDLE_INVALID))
    {
        authum_unused(lh_str_ptr_format_cstr(error, error_size, "HTTP socket: %s",
                                             authum_http_get_error_text()));
        return LH_OS_SYSTEM_NET_SOCKET_HANDLE_INVALID;
    }
    authum_unused(lh_os_system_net_socket_set_reuse_addr(listener, lh_bool_true));
    if (!lh_os_system_net_socket_bind(listener, lh_addr_of(address)) ||
        !lh_os_system_net_socket_listen(listener, 64))
    {
        authum_unused(lh_str_ptr_format_cstr(error, error_size, "cannot listen on %s:%d: %s",
                                             options->bind, options->port,
                                             authum_http_get_error_text()));
        lh_os_system_net_socket_close(listener);
        return LH_OS_SYSTEM_NET_SOCKET_HANDLE_INVALID;
    }
    return listener;
}

lh_os_system_net_socket_handle_t
authum_http_socket_accept(lh_os_system_net_socket_handle_t listener, authum_http_client_t *peer, authum_u32_t *address)
{
    lh_net_ip4_socket_addr_t remote;
    lh_net_ip4_t ip;
    lh_os_system_net_socket_handle_t socket = lh_os_system_net_socket_accept_nonblocking(listener, lh_addr_of(remote));
    if (lh_math_eq(socket, LH_OS_SYSTEM_NET_SOCKET_HANDLE_INVALID))
    {
        return LH_OS_SYSTEM_NET_SOCKET_HANDLE_INVALID;
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
authum_http_socket_receive(lh_os_system_net_socket_handle_t fd, authum_char_t *buffer, authum_usize_t size)
{
    return lh_os_system_net_socket_recv(fd, buffer, size);
}

authum_i64_t
authum_http_socket_send(lh_os_system_net_socket_handle_t fd, const authum_char_t *buffer, authum_usize_t size)
{
    return lh_os_system_net_socket_send(fd, buffer, size);
}

authum_i64_t
authum_http_socket_send_file(lh_os_system_net_socket_handle_t socket, lh_os_system_fs_file_handle_t file, lh_u64_t *offset,
                             authum_usize_t size)
{
    return lh_os_system_net_socket_send_file(socket, file, offset, size);
}

const authum_char_t *
authum_http_get_error_text(void)
{
    const authum_char_t *text = lh_cast_reinterpret(const authum_char_t *, lh_os_get_last_error_desc());

    return authum_null_eq(text) ? "unknown error" : text;
}

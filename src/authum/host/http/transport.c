#include <authum/host/http/internal.h>
#include <lh/core/char/map.h>
#include <lh/core/memory/std.h>
#include <lh/os.h>
#include <lh/os/error/code.h>
#include <authum/null.h>

authum_i64_t
authum_http_now_ms(void)
{
    return authum_clock_get_monotonic_ms(authum_clock_host());
}

authum_bool_t
authum_http_listener_open(lh_os_net_socket_t *listener, const authum_http_options_t *options,
                          authum_char_t *error, authum_u32_t error_size)
{
    lh_net_ip4_t ip;
    if (!lh_net_ip4_parse(options->bind, lh_str_ptr_len(options->bind), lh_addr_of(ip)))
    {
        authum_unused(lh_str_ptr_format_cstr(error, error_size, "bind '%s' must be an IPv4 address",
                                             options->bind));
        return authum_bool_false;
    }
    lh_net_ip4_socket_addr_t address;
    lh_net_ip4_socket_addr_init(lh_addr_of(address), lh_addr_of(ip),
                                lh_cast_static(lh_net_port_t, options->port));
    authum_http_error_reset();
    if (!lh_os_net_socket_open_nonblocking(listener, lh_net_socket_type_tcp))
    {
        authum_unused(lh_str_ptr_format_cstr(error, error_size, "HTTP socket: %s",
                                             authum_http_get_error_text()));
        return authum_bool_false;
    }
    /* The port is ours alone, and a restart may take it over from connections
     * still in TIME_WAIT: SO_REUSEADDR on POSIX, SO_EXCLUSIVEADDRUSE on Windows,
     * where SO_REUSEADDR would let another process bind the same port and take
     * the connections. */
    authum_unused(lh_os_net_socket_set_exclusive_addr(listener, lh_bool_true));
    authum_http_error_reset();
    if (!lh_os_net_socket_bind(listener, lh_addr_of(address)) ||
        !lh_os_net_socket_listen(listener, 64))
    {
        authum_unused(lh_str_ptr_format_cstr(error, error_size, "cannot listen on %s:%d: %s",
                                             options->bind, options->port,
                                             authum_http_get_error_text()));
        lh_os_net_socket_close(listener);
        return authum_bool_false;
    }
    return authum_bool_true;
}

authum_bool_t
authum_http_socket_accept(lh_os_net_socket_t *listener, authum_http_client_t *peer,
                          authum_u32_t *address)
{
    lh_net_ip4_socket_addr_t remote;
    lh_os_net_socket_init(lh_addr_of(peer->fd));
    if (!lh_os_net_socket_accept_nonblocking(listener, lh_addr_of(peer->fd), lh_addr_of(remote)))
    {
        return authum_bool_false;
    }
    lh_net_ip4_t ip;
    lh_net_ip4_socket_addr_get_ip(lh_addr_of(remote), lh_addr_of(ip));
    /* Keep the existing network-order byte key used by the per-peer limits. */
    lh_memory_std_copy(address, ip.octets, sizeof lh_ptr_deref(address));
    peer->port = lh_net_ip4_socket_addr_get_port(lh_addr_of(remote));
    lh_usize_t size = lh_net_ip4_format(lh_addr_of(ip), peer->ip, lh_math_sub(sizeof peer->ip, 1));
    peer->ip[size] = lh_char_map_nul;
    return authum_bool_true;
}

authum_i64_t
authum_http_socket_receive(lh_os_net_socket_t *socket, authum_char_t *buffer, authum_usize_t size)
{
    return lh_os_net_socket_recv(socket, buffer, size);
}

authum_i64_t
authum_http_socket_send(lh_os_net_socket_t *socket, const authum_char_t *buffer,
                        authum_usize_t size)
{
    return lh_os_net_socket_send(socket, buffer, size);
}

authum_i64_t
authum_http_socket_send_file(lh_os_net_socket_t *socket, const lh_os_fs_file_t *file,
                             lh_u64_t *offset, authum_usize_t size)
{
    return lh_os_net_socket_send_file(socket, file, offset, size);
}

void
authum_http_error_reset(void)
{
    lh_os_system_error_t empty;
    lh_os_system_error_init_by_empty(lh_addr_of(empty));
    lh_os_system_set_last_error(empty);
    lh_os_set_last_error_lit(lh_os_error_code_ok, "");
}

const authum_char_t *
authum_http_get_error_text(void)
{
    /* Only the owner thread (server create) asks; one buffer is enough. */
    static authum_char_t text[320];
    const lh_os_system_error_code_t code = lh_os_system_get_last_error_code();
    const authum_char_t *native =
        lh_cast_reinterpret(const authum_char_t *, lh_os_system_get_last_error_desc());
    const authum_char_t *own = lh_cast_reinterpret(const authum_char_t *, lh_os_get_last_error_desc());
    if (lh_math_ne(code, lh_os_system_error_code_ok))
    {
        authum_unused(lh_str_ptr_format_cstr(text, sizeof text, "%s (os error %u)",
                                             native && *native ? native : "no description",
                                             lh_cast_static(lh_uint_t, code)));
        return text;
    }
    if (own && *own)
    {
        authum_unused(lh_str_ptr_format_cstr(text, sizeof text, "%s (code %u)", own,
                                             lh_cast_static(lh_uint_t, lh_os_get_last_error_code())));
        return text;
    }
    return "unknown error";
}

#define _GNU_SOURCE
#include "internal.h"
#include <lh/os/system.h>

authum_i64_t
authum_http_now_ms(void)
{
    return authum_clock_monotonic_ms(authum_clock_host());
}

int
authum_http_listener_open(const authum_http_options_t *options, char *error, unsigned error_size)
{
    lh_net_ip4_t ip;
    lh_net_ip4_socket_addr_t address;
    if (!lh_net_ip4_parse(options->bind, strlen(options->bind), &ip))
    {
        snprintf(error, error_size, "bind '%s' must be an IPv4 address", options->bind);
        return -1;
    }
    lh_net_ip4_socket_addr_init(&address, &ip, (lh_net_port_t)options->port);
    int listener = (int)lh_os_system_net_socket_open_nonblocking(lh_os_system_net_socket_type_tcp);
    if (listener < 0)
    {
        snprintf(error, error_size, "HTTP socket: %s",
                 strerror((int)lh_os_system_get_last_error_code()));
        return -1;
    }
    (void)lh_os_system_net_socket_set_reuse_addr(listener, lh_bool_true);
    if (!lh_os_system_net_socket_bind(listener, &address) ||
        !lh_os_system_net_socket_listen(listener, 64))
    {
        snprintf(error, error_size, "cannot listen on %s:%d: %s", options->bind, options->port,
                 strerror((int)lh_os_system_get_last_error_code()));
        lh_os_system_net_socket_close(listener);
        return -1;
    }
    return listener;
}

int
authum_http_socket_accept(int listener, authum_http_client_t *peer, authum_u32_t *address)
{
    lh_net_ip4_socket_addr_t remote;
    lh_net_ip4_t ip;
    int socket = (int)lh_os_system_net_socket_accept_nonblocking(listener, &remote);
    if (socket < 0)
    {
        return -1;
    }
    lh_net_ip4_socket_addr_get_ip(&remote, &ip);
    /* Keep the existing network-order byte key used by the per-peer limits. */
    memcpy(address, ip.octets, sizeof *address);
    peer->port = lh_net_ip4_socket_addr_get_port(&remote);
    lh_usize_t size = lh_net_ip4_format(&ip, peer->ip, sizeof peer->ip - 1);
    peer->ip[size] = '\0';
    return socket;
}

authum_i64_t
authum_http_socket_receive(int fd, char *buffer, authum_usize_t size)
{
    return lh_os_system_net_socket_recv(fd, buffer, size);
}

authum_i64_t
authum_http_socket_send(int fd, const char *buffer, authum_usize_t size)
{
    return lh_os_system_net_socket_send(fd, buffer, size);
}

authum_i64_t
authum_http_socket_send_file(int socket, int file, off_t *offset, authum_usize_t size)
{
    if (*offset < 0)
    {
        return AUTHUM_HTTP_IO_ERROR;
    }
    lh_u64_t next = (lh_u64_t)*offset;
    lh_ssize_t result = lh_os_system_net_socket_send_file(socket, file, &next, size);
    *offset = (off_t)next;
    return result;
}

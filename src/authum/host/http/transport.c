#define _GNU_SOURCE
#include "internal.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/socket.h>

authum_i64_t
authum_http_now_ms(void)
{
    return authum_clock_monotonic_ms(authum_clock_host());
}

int
authum_http_listener_open(const authum_http_options_t *options, char *error, unsigned error_size)
{
    int listener = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (listener < 0)
    {
        snprintf(error, error_size, "HTTP socket: %s", strerror(errno));
        return -1;
    }

    int reuse = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse);
    struct sockaddr_in address = {.sin_family = AF_INET, .sin_port = htons(options->port)};
    if (inet_pton(AF_INET, options->bind, &address.sin_addr) != 1)
    {
        snprintf(error, error_size, "bind '%s' must be an IPv4 address", options->bind);
        close(listener);
        return -1;
    }
    if (bind(listener, (struct sockaddr *)&address, sizeof address) || listen(listener, 64))
    {
        snprintf(error, error_size, "cannot listen on %s:%d: %s", options->bind, options->port,
                 strerror(errno));
        close(listener);
        return -1;
    }
    return listener;
}

int
authum_http_socket_accept(int listener, authum_http_client_t *peer, authum_u32_t *address)
{
    struct sockaddr_in remote;
    socklen_t length = sizeof remote;
    int socket =
        accept4(listener, (struct sockaddr *)&remote, &length, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (socket < 0)
    {
        return -1;
    }
    *address = remote.sin_addr.s_addr;
    peer->port = ntohs(remote.sin_port);
    inet_ntop(AF_INET, &remote.sin_addr, peer->ip, sizeof peer->ip);
    return socket;
}

static authum_i64_t
io_result(ssize_t result)
{
    if (result >= 0)
    {
        return result;
    }
    return errno == EAGAIN || errno == EINTR ? AUTHUM_HTTP_IO_PENDING : AUTHUM_HTTP_IO_ERROR;
}

authum_i64_t
authum_http_socket_receive(int fd, char *buffer, authum_usize_t size)
{
    return io_result(recv(fd, buffer, size, 0));
}

authum_i64_t
authum_http_socket_send(int fd, const char *buffer, authum_usize_t size)
{
    return io_result(send(fd, buffer, size, MSG_NOSIGNAL));
}

authum_i64_t
authum_http_socket_send_file(int socket, int file, off_t *offset, authum_usize_t size)
{
    return io_result(sendfile(socket, file, offset, size));
}

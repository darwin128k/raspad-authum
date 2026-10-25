#include "internal.h"

void
authum_http_access_record(authum_http_server_t *server, const authum_http_client_t *client,
                          const char *event)
{
    if (!server->options.access_log)
    {
        return;
    }
    unsigned write_index = atomic_load_explicit(&server->log_write, memory_order_relaxed);
    unsigned read_index = atomic_load_explicit(&server->log_read, memory_order_acquire);
    if (write_index - read_index >= AUTHUM_HTTP_LOG_CAPACITY)
    {
        atomic_fetch_add_explicit(&server->log_dropped, 1, memory_order_relaxed);
        return;
    }
    snprintf(server->logs[write_index % AUTHUM_HTTP_LOG_CAPACITY], sizeof server->logs[0],
             "http event=%s id=%u ip=%s:%u method=%s path=\"%s\" status=%d bytes=%llu source=%u "
             "duration_ms=%lld",
             event, client->id, client->ip, client->port, *client->method ? client->method : "-",
             *client->path ? client->path : "-", client->status,
             (unsigned long long)client->body_bytes, client->source,
             (long long)(authum_http_now_ms() - client->born));
    atomic_store_explicit(&server->log_write, write_index + 1, memory_order_release);
}

void
authum_http_access_token(char *dst, size_t capacity, const char *src, size_t length)
{
    size_t n = length < capacity - 1 ? length : capacity - 1;
    for (size_t i = 0; i < n; i++)
    {
        unsigned char x = (unsigned char)src[i];
        dst[i] = (x >= 33 && x <= 126 && x != '"' && x != '\\') ? (char)x : '_';
    }
    if (length >= capacity && n >= 3)
    {
        memcpy(dst + n - 3, "...", 3);
    }
    dst[n] = 0;
}

unsigned
authum_http_server_drain(authum_http_server_t *server, authum_http_log_fn sink, void *user,
                         unsigned limit)
{
    if (!server || !sink || !limit)
    {
        return 0;
    }
    if (limit > AUTHUM_HTTP_LOG_CAPACITY + 1)
    {
        limit = AUTHUM_HTTP_LOG_CAPACITY + 1;
    }
    unsigned count = 0;
    unsigned dropped = atomic_exchange_explicit(&server->log_dropped, 0, memory_order_relaxed);
    if (dropped)
    {
        char line[128];
        snprintf(line, sizeof line, "http event=log_overflow dropped=%u (access log queue full)",
                 dropped);
        sink(line, user);
        ++count;
    }
    while (count < limit && !server->close_pending)
    {
        unsigned read_index = atomic_load_explicit(&server->log_read, memory_order_relaxed);
        unsigned write_index = atomic_load_explicit(&server->log_write, memory_order_acquire);
        if (read_index == write_index)
        {
            break;
        }
        authum_char_t line[AUTHUM_HTTP_LOG_LINE_CAPACITY];
        memcpy(line, server->logs[read_index % AUTHUM_HTTP_LOG_CAPACITY], sizeof line);
        atomic_store_explicit(&server->log_read, read_index + 1, memory_order_release);
        sink(line, user);
        ++count;
    }
    return count;
}

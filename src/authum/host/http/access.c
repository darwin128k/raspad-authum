#include <authum/host/http/internal.h>
#include <lh/char/map.h>
#include <lh/memory/std.h>

void
authum_http_access_record(authum_http_server_t *server, const authum_http_client_t *client,
                          const authum_char_t *event)
{
    if (!server->options.access_log)
    {
        return;
    }
    authum_u32_t write_index =
        atomic_load_explicit(lh_addr_of(server->log_write), memory_order_relaxed);
    authum_u32_t read_index =
        atomic_load_explicit(lh_addr_of(server->log_read), memory_order_acquire);
    if (lh_math_ge(lh_math_sub(write_index, read_index), AUTHUM_HTTP_LOG_CAPACITY))
    {
        atomic_fetch_add_explicit(lh_addr_of(server->log_dropped), 1, memory_order_relaxed);
        return;
    }
    snprintf(server->logs[lh_math_mod(write_index, AUTHUM_HTTP_LOG_CAPACITY)],
             sizeof server->logs[0],
             "http event=%s id=%u ip=%s:%u method=%s path=\"%s\" status=%d bytes=%llu source=%u "
             "duration_ms=%lld",
             event, client->id, client->ip, client->port,
             lh_ptr_deref(client->method) ? client->method : "-",
             lh_ptr_deref(client->path) ? client->path : "-", client->status,
             lh_cast_static(authum_u64_t, client->body_bytes), client->source,
             lh_cast_static(authum_i64_t, (lh_math_sub(authum_http_now_ms(), client->born))));
    atomic_store_explicit(lh_addr_of(server->log_write), lh_math_add(write_index, 1),
                          memory_order_release);
}

void
authum_http_access_token(authum_char_t *dst, authum_usize_t capacity, const authum_char_t *src,
                         authum_usize_t length)
{
    authum_usize_t n =
        lh_math_lt(length, lh_math_sub(capacity, 1)) ? length : lh_math_sub(capacity, 1);
    for (authum_usize_t i = 0; lh_math_lt(i, n); i = lh_math_add_one(i))
    {
        authum_byte_t x = lh_cast_static(authum_byte_t, src[i]);
        dst[i] = (lh_math_ge(x, 33) && lh_math_le(x, 126) && lh_math_ne(x, lh_char_map_dquot) &&
                  lh_math_ne(x, lh_char_map_backslash))
                     ? lh_cast_static(authum_char_t, x)
                     : lh_char_map_underscore;
    }
    if (lh_math_ge(length, capacity) && lh_math_ge(n, 3))
    {
        lh_memory_std_copy(lh_math_sub(lh_math_add(dst, n), 3), "...", 3);
    }
    dst[n] = 0;
}

authum_u32_t
authum_http_server_drain(authum_http_server_t *server, authum_http_log_fn sink, authum_ptr user,
                         authum_u32_t limit)
{
    if (!server || !sink || !limit)
    {
        return 0;
    }
    if (lh_math_gt(limit, lh_math_add(AUTHUM_HTTP_LOG_CAPACITY, 1)))
    {
        limit = lh_math_add(AUTHUM_HTTP_LOG_CAPACITY, 1);
    }
    authum_u32_t count = 0;
    authum_u32_t dropped =
        atomic_exchange_explicit(lh_addr_of(server->log_dropped), 0, memory_order_relaxed);
    if (dropped)
    {
        authum_char_t line[128];
        snprintf(line, sizeof line, "http event=log_overflow dropped=%u (access log queue full)",
                 dropped);
        sink(line, user);
        count = lh_math_add_one(count);
    }
    while (lh_math_lt(count, limit) && !server->close_pending)
    {
        authum_u32_t read_index =
            atomic_load_explicit(lh_addr_of(server->log_read), memory_order_relaxed);
        authum_u32_t write_index =
            atomic_load_explicit(lh_addr_of(server->log_write), memory_order_acquire);
        if (lh_math_eq(read_index, write_index))
        {
            break;
        }
        authum_char_t line[AUTHUM_HTTP_LOG_LINE_CAPACITY];
        lh_memory_std_copy(line, server->logs[lh_math_mod(read_index, AUTHUM_HTTP_LOG_CAPACITY)],
                           sizeof line);
        atomic_store_explicit(lh_addr_of(server->log_read), lh_math_add(read_index, 1),
                              memory_order_release);
        sink(line, user);
        count = lh_math_add_one(count);
    }
    return count;
}

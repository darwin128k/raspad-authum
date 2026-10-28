#include <authum/host/http/internal.h>
#include <authum/byte.h>
#include <authum/char.h>
#include <authum/size.h>
#include <lh/cast/static.h>

void
authum_http_response_set(authum_http_client_t *client, authum_i32_t code,
                         const authum_char_t *reason, off_t size, authum_i32_t head)
{
    client->status = code;
    client->output_size = lh_cast_static(
        authum_usize_t, snprintf(client->output, sizeof client->output,
                                 "HTTP/1.0 %d %s\r\nContent-Length: %lld\r\nContent-Type: "
                                 "application/octet-stream\r\nConnection: "
                                 "close\r\nX-Content-Type-Options: nosniff\r\n\r\n",
                                 code, reason, lh_cast_static(authum_i64_t, size)));
    client->sent = 0;
    client->state = authum_http_client_writing;
    client->remaining = head ? 0 : size;
}

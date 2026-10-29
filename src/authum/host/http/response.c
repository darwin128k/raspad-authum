#include <authum/host/http/internal.h>
#include <lh/char/map.h>
#include <lh/str/format/ullong.h>

void
authum_http_response_set(authum_http_client_t *client, authum_i32_t code,
                         const authum_char_t *reason, lh_u64_t size, authum_i32_t head)
{
    authum_char_t length[24];

    length[lh_str_ptr_format_ullong(lh_cast_static(lh_ullong_t, size), length,
                                    sizeof length - 1u)] = lh_char_map_nul;
    client->status = code;
    client->output_size = lh_str_ptr_format_cstr(
        client->output, sizeof client->output,
        "HTTP/1.0 %d %s\r\nContent-Length: %s\r\nContent-Type: "
        "application/octet-stream\r\nConnection: "
        "close\r\nX-Content-Type-Options: nosniff\r\n\r\n",
        code, reason, length);
    client->sent = 0;
    client->state = authum_http_client_writing;
    client->remaining = head ? 0 : size;
}

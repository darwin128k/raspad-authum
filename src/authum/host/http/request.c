#include "internal.h"
#include "picohttpparser.h"
#include <strings.h>

void
authum_http_request_read(authum_http_client_t *client, const int *roots,
                         const authum_http_options_t *options)
{
    const char *method, *path;
    size_t method_length, path_length, header_count = 64;
    int minor_version;
    struct phr_header headers[64];
    int parsed = phr_parse_request(client->input, client->used, &method, &method_length, &path,
                                   &path_length, &minor_version, headers, &header_count, 0);
    if (parsed == -2 && client->used < sizeof client->input)
    {
        return;
    }
    if (parsed < 0 || (size_t)parsed != client->used || minor_version > 1)
    {
        authum_http_response_set(client, 400, "Bad Request", 0, 1);
        return;
    }
    authum_http_access_token(client->method, sizeof client->method, method, method_length);
    authum_http_access_token(client->path, sizeof client->path, path, path_length);
    int head = method_length == 4 && !memcmp(method, "HEAD", 4);
    if (!head && !(method_length == 3 && !memcmp(method, "GET", 3)))
    {
        authum_http_response_set(client, 405, "Method Not Allowed", 0, 1);
        return;
    }
    for (size_t i = 0; i < header_count; i++)
    {
        if (!headers[i].name ||
            (headers[i].name_len == 17 && !strncasecmp(headers[i].name, "Transfer-Encoding", 17)) ||
            (headers[i].name_len == 14 && !strncasecmp(headers[i].name, "Content-Length", 14)))
        {
            authum_http_response_set(client, 400, "Bad Request", 0, 1);
            return;
        }
    }
    off_t length = 0;
    for (unsigned i = 0; i < options->source_count; i++)
    {
        client->file = authum_http_resource_open(roots[i], options, path, path_length, &length);
        if (client->file >= 0)
        {
            client->source = i + 1;
            break;
        }
    }
    if (client->file < 0)
    {
        authum_http_response_set(client, 404, "Not Found", 0, 1);
    }
    else
    {
        authum_http_response_set(client, 200, "OK", length, head);
    }
}

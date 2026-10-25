#define _GNU_SOURCE
#include "internal.h"
#include <fcntl.h>
#include <sys/stat.h>

int
authum_http_policy_contains(const char *list, const char *value, size_t n, int prefix)
{
    while (*list)
    {
        while (*list == ' ')
        {
            ++list;
        }
        const char *e = strchr(list, ' ');
        size_t z = e ? (size_t)(e - list) : strlen(list);
        if (z && (prefix ? n >= z && !memcmp(list, value, z) && (n == z || value[z] == '/')
                         : z == n && !memcmp(list, value, n)))
        {
            return 1;
        }
        list += z;
        if (!*list)
        {
            break;
        }
    }
    return 0;
}

int
authum_http_resource_open(int root, const authum_http_options_t *options, const char *url,
                          size_t len, off_t *length)
{
    char path[2048];
    if (len < 2 || len >= sizeof path || url[0] != '/')
    {
        return -1;
    }
    for (size_t i = 1; i < len; i++)
    {
        unsigned char x = url[i];
        if (!((x >= 'a' && x <= 'z') || (x >= 'A' && x <= 'Z') || (x >= '0' && x <= '9') ||
              x == '_' || x == '-' || x == '.' || x == '/' || x == '$'))
        {
            return -1;
        }
    }
    memcpy(path, url + 1, len - 1);
    path[len - 1] = 0;
    if (authum_http_policy_contains(options->deny_prefixes, path, len - 1, 1))
    {
        return -1;
    }
    char *slash = strchr(path, '/');
    if (slash && !authum_http_policy_contains(options->allow_dirs, path, (size_t)(slash - path), 0))
    {
        return -1;
    }
    /* Root files have a separate explicit extension allow-list. */
    const char *ext = strrchr(path, '.');
    if (!ext ||
        !authum_http_policy_contains(options->allow_extensions, ext + 1, strlen(ext + 1), 0) ||
        (!slash &&
         !authum_http_policy_contains(options->root_extensions, ext + 1, strlen(ext + 1), 0)))
    {
        return -1;
    }
    int dir = dup(root);
    if (dir < 0)
    {
        return -1;
    }
    char *part = path;
    for (;;)
    {
        char *end = strchr(part, '/');
        if (end)
        {
            *end = 0;
        }
        if (!*part || part[0] == '.')
        {
            close(dir);
            return -1;
        }
        int next = openat(dir, part,
                          O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK | (end ? O_DIRECTORY : 0));
        close(dir);
        if (next < 0)
        {
            return -1;
        }
        if (!end)
        {
            struct stat st;
            if (fstat(next, &st) || !S_ISREG(st.st_mode) || st.st_size < 0 ||
                (authum_u64_t)st.st_size > options->max_file_bytes)
            {
                close(next);
                return -1;
            }
            *length = st.st_size;
            return next;
        }
        dir = next;
        part = end + 1;
    }
}

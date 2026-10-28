#include "internal.h"
#include <lh/os/system/fs/file.h>

static authum_bool_t
authum_http_policy_contains(const authum_char_t *list, const authum_char_t *value, authum_usize_t n,
                            authum_bool_t prefix)
{
    while (*list)
    {
        while (*list == ' ')
        {
            ++list;
        }
        const authum_char_t *e = strchr(list, ' ');
        authum_usize_t z = e ? (authum_usize_t)(e - list) : (authum_usize_t)strlen(list);
        if (z && (prefix ? n >= z && !memcmp(list, value, z) && (n == z || value[z] == '/')
                         : z == n && !memcmp(list, value, n)))
        {
            return authum_bool_true;
        }
        list += z;
        if (!*list)
        {
            break;
        }
    }
    return authum_bool_false;
}

authum_i32_t
authum_http_resource_open(authum_i32_t root, const authum_http_options_t *options,
                          const authum_char_t *url, authum_usize_t len, off_t *length)
{
    authum_char_t path[2048];
    if (len < 2 || len >= sizeof path || url[0] != '/')
    {
        return -1;
    }
    for (authum_usize_t i = 1; i < len; i++)
    {
        authum_char_t x = url[i];
        /* Hidden names are HTTP policy; LH only rejects dot traversal. */
        if (x == '.' && url[i - 1] == '/')
        {
            return -1;
        }
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
    authum_char_t *slash = strchr(path, '/');
    if (slash &&
        !authum_http_policy_contains(options->allow_dirs, path, (authum_usize_t)(slash - path), 0))
    {
        return -1;
    }
    /* Root files have a separate explicit extension allow-list. */
    const authum_char_t *ext = strrchr(path, '.');
    if (!ext ||
        !authum_http_policy_contains(options->allow_extensions, ext + 1,
                                     (authum_usize_t)strlen(ext + 1), 0) ||
        (!slash && !authum_http_policy_contains(options->root_extensions, ext + 1,
                                                (authum_usize_t)strlen(ext + 1), 0)))
    {
        return -1;
    }
    lh_fs_size_t size;
    lh_os_system_fs_file_handle_t file = lh_os_system_fs_file_open_read_at(root, path, &size);
    if (file == LH_OS_SYSTEM_FS_FILE_HANDLE_INVALID)
    {
        return -1;
    }
    if (size > options->max_file_bytes)
    {
        lh_os_system_fs_file_close(file);
        return -1;
    }
    /* The Linux HTTP transport still uses the native sendfile offset type. */
    *length = (off_t)size;
    return (authum_i32_t)file;
}

#include <authum/host/http/internal.h>
#include <lh/char/letter.h>
#include <lh/char/digit.h>
#include <lh/memory/std.h>
#include <lh/os/system/fs/file.h>

static authum_bool_t
authum_http_policy_contains(const authum_char_t *list, const authum_char_t *value, authum_usize_t n,
                            authum_bool_t prefix)
{
    while (lh_ptr_deref(list))
    {
        while (lh_math_eq(lh_ptr_deref(list), lh_char_map_space))
        {
            list = lh_math_add_one(list);
        }
        const authum_char_t *e = strchr(list, lh_char_map_space);
        authum_usize_t z = e ? (authum_usize_t)(lh_math_sub(e, list))
                             : lh_cast_static(authum_usize_t, strlen(list));
        if (z && (prefix ? lh_math_ge(n, z) && !lh_memory_std_compare(list, value, z) &&
                               (lh_math_eq(n, z) || lh_math_eq(value[z], lh_char_map_slash))
                         : lh_math_eq(z, n) && !lh_memory_std_compare(list, value, n)))
        {
            return authum_bool_true;
        }
        list = lh_math_add(list, z);
        if (!lh_ptr_deref(list))
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
    if (lh_math_lt(len, 2) || lh_math_ge(len, sizeof path) || lh_math_ne(url[0], lh_char_map_slash))
    {
        return -1;
    }
    for (authum_usize_t i = 1; lh_math_lt(i, len); i = lh_math_add_one(i))
    {
        authum_char_t x = url[i];
        /* Hidden names are HTTP policy; LH only rejects dot traversal. */
        if (lh_math_eq(x, lh_char_map_period) &&
            lh_math_eq(url[lh_math_sub(i, 1)], lh_char_map_slash))
        {
            return -1;
        }
        if (!(lh_char_is_letter(x) || lh_char_is_digit(x) ||
              lh_math_eq(x, lh_char_map_underscore) || lh_math_eq(x, lh_char_map_minus) ||
              lh_math_eq(x, lh_char_map_period) || lh_math_eq(x, lh_char_map_slash) ||
              lh_math_eq(x, lh_char_map_dollar)))
        {
            return -1;
        }
    }
    lh_memory_std_copy(path, lh_math_add(url, 1), lh_math_sub(len, 1));
    path[lh_math_sub(len, 1)] = 0;
    if (authum_http_policy_contains(options->deny_prefixes, path, lh_math_sub(len, 1), 1))
    {
        return -1;
    }
    authum_char_t *slash = strchr(path, lh_char_map_slash);
    if (slash && !authum_http_policy_contains(options->allow_dirs, path,
                                              (authum_usize_t)(lh_math_sub(slash, path)), 0))
    {
        return -1;
    }
    /* Root files have a separate explicit extension allow-list. */
    const authum_char_t *ext = strrchr(path, lh_char_map_period);
    if (!ext ||
        !authum_http_policy_contains(options->allow_extensions, lh_math_add(ext, 1),
                                     lh_cast_static(authum_usize_t, strlen(lh_math_add(ext, 1))),
                                     0) ||
        (!slash && !authum_http_policy_contains(
                       options->root_extensions, lh_math_add(ext, 1),
                       lh_cast_static(authum_usize_t, strlen(lh_math_add(ext, 1))), 0)))
    {
        return -1;
    }
    lh_fs_size_t size;
    lh_os_system_fs_file_handle_t file =
        lh_os_system_fs_file_open_read_at(root, path, lh_addr_of(size));
    if (lh_math_eq(file, LH_OS_SYSTEM_FS_FILE_HANDLE_INVALID))
    {
        return -1;
    }
    if (lh_math_gt(size, options->max_file_bytes))
    {
        lh_os_system_fs_file_close(file);
        return -1;
    }
    /* The Linux HTTP transport still uses the native sendfile offset type. */
    lh_ptr_deref(length) = lh_cast_static(off_t, size);
    return lh_cast_static(authum_i32_t, file);
}

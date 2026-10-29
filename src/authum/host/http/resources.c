#include <authum/host/http/internal.h>
#include <lh/char/letter.h>
#include <lh/char/digit.h>
#include <lh/memory/std.h>

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
        const authum_usize_t rest = lh_str_ptr_len(list);
        const authum_char_t *e = lh_str_ptr_find_of_char(list, rest, lh_char_map_space);
        authum_usize_t z = e ? (authum_usize_t)(lh_math_sub(e, list)) : rest;
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

lh_os_system_fs_file_handle_t
authum_http_resource_open(lh_os_system_fs_file_handle_t root, const authum_http_options_t *options,
                          const authum_char_t *url, authum_usize_t len, lh_u64_t *length)
{
    authum_char_t path[2048];
    if (lh_math_lt(len, 2) || lh_math_ge(len, sizeof path) || lh_math_ne(url[0], lh_char_map_slash))
    {
        return LH_OS_SYSTEM_FS_FILE_HANDLE_INVALID;
    }
    for (authum_usize_t i = 1; lh_math_lt(i, len); i = lh_math_add_one(i))
    {
        authum_char_t x = url[i];
        /* Hidden names are HTTP policy; LH only rejects dot traversal. */
        if (lh_math_eq(x, lh_char_map_period) &&
            lh_math_eq(url[lh_math_sub(i, 1)], lh_char_map_slash))
        {
            return LH_OS_SYSTEM_FS_FILE_HANDLE_INVALID;
        }
        if (!(lh_char_is_letter(x) || lh_char_is_digit(x) ||
              lh_math_eq(x, lh_char_map_underscore) || lh_math_eq(x, lh_char_map_minus) ||
              lh_math_eq(x, lh_char_map_period) || lh_math_eq(x, lh_char_map_slash) ||
              lh_math_eq(x, lh_char_map_dollar)))
        {
            return LH_OS_SYSTEM_FS_FILE_HANDLE_INVALID;
        }
    }
    lh_memory_std_copy(path, lh_math_add(url, 1), lh_math_sub(len, 1));
    path[lh_math_sub(len, 1)] = 0;
    if (authum_http_policy_contains(options->deny_prefixes, path, lh_math_sub(len, 1), 1))
    {
        return LH_OS_SYSTEM_FS_FILE_HANDLE_INVALID;
    }
    const authum_char_t *slash =
        lh_str_ptr_find_of_char(path, lh_math_sub(len, 1), lh_char_map_slash);
    if (slash && !authum_http_policy_contains(options->allow_dirs, path,
                                              (authum_usize_t)(lh_math_sub(slash, path)), 0))
    {
        return LH_OS_SYSTEM_FS_FILE_HANDLE_INVALID;
    }
    /* Root files have a separate explicit extension allow-list. */
    const authum_char_t *ext =
        lh_str_ptr_rfind_of_char(path, lh_math_sub(len, 1), lh_char_map_period);
    if (!ext ||
        !authum_http_policy_contains(options->allow_extensions, lh_math_add(ext, 1),
                                     lh_str_ptr_len(lh_math_add(ext, 1)),
                                     0) ||
        (!slash && !authum_http_policy_contains(
                       options->root_extensions, lh_math_add(ext, 1),
                       lh_str_ptr_len(lh_math_add(ext, 1)), 0)))
    {
        return LH_OS_SYSTEM_FS_FILE_HANDLE_INVALID;
    }
    lh_fs_size_t size;
    lh_os_system_fs_file_handle_t file =
        lh_os_system_fs_file_open_read_at(root, path, lh_addr_of(size));
    if (lh_math_eq(file, LH_OS_SYSTEM_FS_FILE_HANDLE_INVALID))
    {
        return LH_OS_SYSTEM_FS_FILE_HANDLE_INVALID;
    }
    if (lh_math_gt(size, options->max_file_bytes))
    {
        lh_os_system_fs_file_close(file);
        return LH_OS_SYSTEM_FS_FILE_HANDLE_INVALID;
    }
    lh_ptr_deref(length) = lh_cast_static(lh_u64_t, size);
    return file;
}

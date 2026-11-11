#include <authum/host/http/internal.h>
#include <lh/core/char/map.h>
#include <lh/core/http/target.h>
#include <lh/core/memory/std.h>

authum_bool_t
authum_http_policy_contains(const authum_char_t *list, lh_str_view_t value, authum_bool_t prefix)
{
    const authum_char_t *text = lh_str_view_get_data(lh_addr_of(value));
    const authum_usize_t n = lh_str_view_get_size(lh_addr_of(value));

    while (lh_ptr_deref(list))
    {
        while (lh_math_eq(lh_ptr_deref(list), lh_char_map_space))
        {
            list = lh_math_add_one(list);
        }
        const authum_usize_t rest = lh_str_ptr_len(list);
        const authum_char_t *e = lh_str_ptr_find_of_char(list, rest, lh_char_map_space);
        authum_usize_t z =
            !authum_null_eq(e) ? lh_cast_static(authum_usize_t, lh_ptr_udiff(e, list)) : rest;
        if (z &&
            (prefix
                 ? lh_math_ge(n, z) && lh_str_ptr_equals_by_size(list, z, text, z, lh_bool_true) &&
                       (lh_math_eq(n, z) || lh_math_eq(text[z], lh_char_map_slash))
                 : lh_math_eq(z, n) && lh_str_ptr_equals_by_size(list, n, text, n, lh_bool_true)))
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

authum_usize_t
authum_http_resource_check(const authum_http_options_t *options, const authum_char_t *url,
                           authum_usize_t len, authum_char_t *out, authum_usize_t capacity,
                           const authum_char_t **reason)
{
    /* The target as a safe relative path: lh's rules, the same on every platform. */
    lh_str_view_t target;
    lh_str_view_init_by_size(lh_addr_of(target), url, len);
    lh_http_target_error_t error;
    const authum_usize_t n =
        lh_http_target_try_copy_path(lh_addr_of(target), out, capacity, lh_addr_of(error));
    if (lh_math_is_zero(n))
    {
        lh_ptr_deref(reason) = lh_http_target_error_get_name(error);
        return 0;
    }
    lh_str_view_t path;
    lh_str_view_init_by_size(lh_addr_of(path), out, n);
    if (authum_http_policy_contains(options->deny_prefixes, path, 1))
    {
        *reason = "denied_prefix";
        return 0;
    }
    const authum_char_t *slash = lh_str_ptr_find_of_char(out, n, lh_char_map_slash);
    if (!authum_null_eq(slash))
    {
        lh_str_view_t dir;
        lh_str_view_from_offset(lh_addr_of(path), 0,
                                lh_cast_static(authum_usize_t, lh_ptr_udiff(slash, out)),
                                lh_addr_of(dir));
        if (!authum_http_policy_contains(options->allow_dirs, dir, 0))
        {
            *reason = "dir_not_allowed";
            return 0;
        }
    }
    /* The extension of the last component; root files have their own list. */
    const authum_char_t *last = lh_str_ptr_rfind_of_char(out, n, lh_char_map_slash);
    last = !authum_null_eq(last) ? last + 1 : out;
    const authum_char_t *ext = lh_str_ptr_rfind_of_char(
        last, lh_cast_static(authum_usize_t, lh_ptr_udiff(out + n, last)), lh_char_map_period);
    lh_str_view_t extension;
    lh_str_view_init_empty(lh_addr_of(extension));
    if (!authum_null_eq(ext))
    {
        const authum_usize_t at = lh_cast_static(authum_usize_t, lh_ptr_udiff(ext + 1, out));
        lh_str_view_from_offset(lh_addr_of(path), at, lh_math_sub(n, at), lh_addr_of(extension));
    }
    if (lh_str_view_is_empty(lh_addr_of(extension)) ||
        !authum_http_policy_contains(options->allow_extensions, extension, 0) ||
        (authum_null_eq(slash) &&
         !authum_http_policy_contains(options->root_extensions, extension, 0)))
    {
        *reason = "ext_not_allowed";
        return 0;
    }
    return n;
}

authum_bool_t
authum_http_resource_open(lh_os_fs_file_t *file, const lh_os_fs_file_t *root,
                          const authum_http_options_t *options, const authum_char_t *path,
                          lh_u64_t *length, const authum_char_t **reason)
{
    lh_fs_path_t relative;
    lh_fs_path_init(lh_addr_of(relative));
    lh_str_view_t text;
    lh_str_view_init(lh_addr_of(text), path);
    lh_fs_path_set(lh_addr_of(relative), text, lh_fs_path_style_posix);
    lh_fs_size_t size;
    authum_bool_t opened =
        lh_os_fs_file_try_open_read_at(file, root, lh_addr_of(relative), lh_addr_of(size));
    lh_fs_path_deinit(lh_addr_of(relative));
    if (!opened)
    {
        *reason = "not_found";
        return authum_bool_false;
    }
    if (lh_math_gt(size, options->max_file_bytes))
    {
        lh_os_fs_file_close(file);
        *reason = "too_large";
        return authum_bool_false;
    }
    lh_ptr_deref(length) = lh_cast_static(lh_u64_t, size);
    return authum_bool_true;
}

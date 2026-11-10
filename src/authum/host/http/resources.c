#include <authum/host/http/internal.h>
#include <lh/core/char/xdigit.h>
#include <lh/core/memory/std.h>

authum_bool_t
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
        if (z && (prefix ? lh_math_ge(n, z) && lh_str_ptr_equals_by_size(list, z, value, z, lh_bool_true) &&
                               (lh_math_eq(n, z) || lh_math_eq(value[z], lh_char_map_slash))
                         : lh_math_eq(z, n) && lh_str_ptr_equals_by_size(list, n, value, n, lh_bool_true)))
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

/* A byte a published name may hold once decoded. Separators, controls and the
 * characters Windows reserves are refused on every platform. */
authum_bool_t
authum_http_name_byte(authum_byte_t x)
{
    if (x < 0x20 || x == 0x7f)
    {
        return authum_bool_false;
    }
    switch (x)
    {
    case '\\':
    case ':':
    case '*':
    case '?':
    case '"':
    case '<':
    case '>':
    case '|':
        return authum_bool_false;
    default:
        return authum_bool_true;
    }
}

authum_usize_t
authum_http_resource_check(const authum_http_options_t *options, const authum_char_t *url,
                           authum_usize_t len, authum_char_t *out, authum_usize_t capacity,
                           const authum_char_t **reason)
{
    /* The query and fragment never name a file. */
    for (authum_usize_t i = 0; i < len; ++i)
    {
        if (url[i] == '?' || url[i] == '#')
        {
            len = i;
            break;
        }
    }
    if (len < 2 || url[0] != '/')
    {
        *reason = "bad_target";
        return 0;
    }
    /* Decode exactly once; an encoded separator or NUL never becomes one. */
    authum_usize_t n = 0;
    for (authum_usize_t i = 1; i < len; ++i)
    {
        authum_byte_t x = (authum_byte_t)url[i];
        if (x == '%')
        {
            if (i + 2 >= len || !lh_char_is_xdigit(url[i + 1]) || !lh_char_is_xdigit(url[i + 2]))
            {
                *reason = "bad_encoding";
                return 0;
            }
            x = (authum_byte_t)(lh_char_to_xdigit(url[i + 1]) * 16 + lh_char_to_xdigit(url[i + 2]));
            i += 2;
            if (x == '/' || x == 0)
            {
                *reason = "encoded_separator";
                return 0;
            }
        }
        if (x != '/' && !authum_http_name_byte(x))
        {
            *reason = "bad_char";
            return 0;
        }
        if (n + 1 >= capacity)
        {
            *reason = "uri_too_long";
            return 0;
        }
        out[n++] = (authum_char_t)x;
    }
    out[n] = 0;
    /* Components: no empty ones, no hidden or dot names, nothing Windows would
     * trim or alias (trailing dot/space, 8.3 short names like NAME~1). */
    authum_usize_t start = 0;
    for (authum_usize_t i = 0; i <= n; ++i)
    {
        if (i < n && out[i] != '/')
        {
            if (out[i] == '~' && i + 1 < n && out[i + 1] >= '0' && out[i + 1] <= '9')
            {
                *reason = "short_name";
                return 0;
            }
            continue;
        }
        if (i == start)
        {
            *reason = "empty_component";
            return 0;
        }
        if (out[start] == '.')
        {
            *reason = "hidden_or_dot";
            return 0;
        }
        if (out[i - 1] == '.' || out[i - 1] == ' ')
        {
            *reason = "trailing_dot_or_space";
            return 0;
        }
        start = i + 1;
    }
    if (authum_http_policy_contains(options->deny_prefixes, out, n, 1))
    {
        *reason = "denied_prefix";
        return 0;
    }
    const authum_char_t *slash = lh_str_ptr_find_of_char(out, n, lh_char_map_slash);
    if (slash && !authum_http_policy_contains(options->allow_dirs, out,
                                              (authum_usize_t)(slash - out), 0))
    {
        *reason = "dir_not_allowed";
        return 0;
    }
    /* The extension of the last component; root files have their own list. */
    const authum_char_t *last = lh_str_ptr_rfind_of_char(out, n, lh_char_map_slash);
    last = last ? last + 1 : out;
    const authum_char_t *ext =
        lh_str_ptr_rfind_of_char(last, (authum_usize_t)(out + n - last), lh_char_map_period);
    authum_usize_t ext_len = ext ? (authum_usize_t)(out + n - ext - 1) : 0;
    if (!ext || !ext_len ||
        !authum_http_policy_contains(options->allow_extensions, ext + 1, ext_len, 0) ||
        (!slash && !authum_http_policy_contains(options->root_extensions, ext + 1, ext_len, 0)))
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

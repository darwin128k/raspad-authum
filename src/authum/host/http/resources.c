#include <authum/host/http/internal.h>
#include <lh/core/char/digit.h>
#include <lh/core/memory/std.h>

/* The characters Windows reserves in a name; refused on every platform. */
static const lh_char_t m_http_name_reserved[] = {
    lh_char_map_backslash, lh_char_map_colon, lh_char_map_asterisk, lh_char_map_question,
    lh_char_map_dquot,     lh_char_map_less,  lh_char_map_greater,  lh_char_map_pipe};

/* Where the path of a request target ends: the query or the fragment. */
static const lh_char_t m_http_target_ends[] = {lh_char_map_question, lh_char_map_hash};

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
        authum_usize_t z = e ? lh_cast_static(authum_usize_t, lh_ptr_udiff(e, list)) : rest;
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

/* A byte a published name may hold once decoded. Separators, controls and the
 * characters Windows reserves are refused on every platform. */
authum_bool_t
authum_http_name_byte(authum_byte_t x)
{
    const lh_char_t ch = lh_cast_static(lh_char_t, x);

    return lh_cast_static(authum_bool_t,
                          lh_math_gt(x, lh_char_map_us) && lh_math_ne(x, lh_char_map_del) &&
                              !lh_str_ptr_contains_char(m_http_name_reserved,
                                                        lh_array_ptr_get_size(m_http_name_reserved),
                                                        ch));
}

authum_usize_t
authum_http_resource_check(const authum_http_options_t *options, const authum_char_t *url,
                           authum_usize_t len, authum_char_t *out, authum_usize_t capacity,
                           const authum_char_t **reason)
{
    /* The query and fragment never name a file. */
    const authum_char_t *end = lh_str_ptr_find_of_chars(url, len, m_http_target_ends,
                                                        lh_array_ptr_get_size(m_http_target_ends));
    if (end)
    {
        len = lh_cast_static(authum_usize_t, lh_ptr_udiff(end, url));
    }
    if (len < 2 || url[0] != lh_char_map_slash)
    {
        *reason = "bad_target";
        return 0;
    }
    /* Decode exactly once; an encoded separator or NUL never becomes one. */
    authum_usize_t n = 0;
    for (authum_usize_t i = 1; i < len; ++i)
    {
        authum_byte_t x = lh_cast_static(authum_byte_t, url[i]);
        if (x == lh_char_map_percent)
        {
            if (i + 2 >= len || !lh_str_ptr_parse_hex_bytes(url + i + 1, 2, lh_addr_of(x), 1))
            {
                *reason = "bad_encoding";
                return 0;
            }
            i += 2;
            if (x == lh_char_map_slash || x == lh_char_map_nul)
            {
                *reason = "encoded_separator";
                return 0;
            }
        }
        if (x != lh_char_map_slash && !authum_http_name_byte(x))
        {
            *reason = "bad_char";
            return 0;
        }
        if (n + 1 >= capacity)
        {
            *reason = "uri_too_long";
            return 0;
        }
        out[n++] = lh_cast_static(authum_char_t, x);
    }
    lh_str_ptr_erase(out, n);
    /* Components: no empty ones, no hidden or dot names, nothing Windows would
     * trim or alias (trailing dot/space, 8.3 short names like NAME~1). */
    authum_usize_t start = 0;
    for (authum_usize_t i = 0; i <= n; ++i)
    {
        if (i < n && out[i] != lh_char_map_slash)
        {
            if (out[i] == lh_char_map_tilde && i + 1 < n && lh_char_is_digit(out[i + 1]))
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
        if (out[start] == lh_char_map_period)
        {
            *reason = "hidden_or_dot";
            return 0;
        }
        if (out[i - 1] == lh_char_map_period || out[i - 1] == lh_char_map_space)
        {
            *reason = "trailing_dot_or_space";
            return 0;
        }
        start = i + 1;
    }
    lh_str_view_t path;
    lh_str_view_init_by_size(lh_addr_of(path), out, n);
    if (authum_http_policy_contains(options->deny_prefixes, path, 1))
    {
        *reason = "denied_prefix";
        return 0;
    }
    const authum_char_t *slash = lh_str_ptr_find_of_char(out, n, lh_char_map_slash);
    if (slash)
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
    last = last ? last + 1 : out;
    const authum_char_t *ext = lh_str_ptr_rfind_of_char(
        last, lh_cast_static(authum_usize_t, lh_ptr_udiff(out + n, last)), lh_char_map_period);
    lh_str_view_t extension;
    lh_str_view_init_empty(lh_addr_of(extension));
    if (ext)
    {
        const authum_usize_t at = lh_cast_static(authum_usize_t, lh_ptr_udiff(ext + 1, out));
        lh_str_view_from_offset(lh_addr_of(path), at, lh_math_sub(n, at), lh_addr_of(extension));
    }
    if (lh_str_view_is_empty(lh_addr_of(extension)) ||
        !authum_http_policy_contains(options->allow_extensions, extension, 0) ||
        (!slash && !authum_http_policy_contains(options->root_extensions, extension, 0)))
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

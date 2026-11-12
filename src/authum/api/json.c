#include <authum/api/json/private.h>
#include <authum/unused.h>

/* Read @p json into @p root; the caller deinits @p root whatever this returns. */
authum_bool_t
authum_json_read(const authum_char_t *json, lh_web_json_node_t *root)
{
    lh_web_json_node_init(root);
    if (authum_null_eq(json))
    {
        return authum_bool_false;
    }
    lh_str_view_t text;
    lh_str_view_init(lh_addr_of(text), json);
    lh_web_json_reader_t reader;
    lh_web_json_reader_init(lh_addr_of(reader), text);
    return lh_web_json_reader_read(lh_addr_of(reader), root) ? authum_bool_true : authum_bool_false;
}

/* The member @p key of the object @p root, or null. */
const lh_web_json_node_t *
authum_json_object_find(const lh_web_json_node_t *root, const authum_char_t *key)
{
    if (authum_null_eq(key) || lh_math_eq(key[0], lh_char_map_nul))
    {
        return authum_null;
    }
    lh_str_view_t name;
    lh_str_view_init(lh_addr_of(name), key);
    return lh_web_json_node_get_member(root, name);
}

authum_bool_t
authum_json_object_copy_cstr(const authum_char_t *json, const authum_char_t *key,
                             authum_char_t *out, authum_usize_t out_size)
{
    const lh_web_json_node_t *member;
    authum_bool_t found = authum_bool_false;

    if (authum_null_eq(out) || lh_math_eq(out_size, 0u))
    {
        return authum_bool_false;
    }
    lh_str_ptr_clear(out);

    lh_web_json_node_t root;
    if (authum_json_read(json, lh_addr_of(root)))
    {
        member = authum_json_object_find(lh_addr_of(root), key);
        if (!authum_null_eq(member) && lh_web_json_node_has_kind(member, lh_web_json_kind_string))
        {
            const lh_str_view_t value = lh_web_json_node_get_str(member);

            /* Truncated to the buffer, as before; the text is already unescaped. */
            authum_unused(lh_str_ptr_copy_cstr(out, out_size,
                                               lh_str_view_get_data(lh_addr_of(value)),
                                               lh_str_view_get_size(lh_addr_of(value))));
            found = authum_bool_true;
        }
    }
    lh_web_json_node_deinit(lh_addr_of(root));
    return found;
}

authum_bool_t
authum_json_object_try_get_i32(const authum_char_t *json, const authum_char_t *key,
                               authum_i32_t *out)
{
    const lh_web_json_node_t *member;
    authum_bool_t found = authum_bool_false;

    if (authum_null_eq(out))
    {
        return authum_bool_false;
    }
    lh_web_json_node_t root;
    lh_sllong_t value;
    if (authum_json_read(json, lh_addr_of(root)))
    {
        member = authum_json_object_find(lh_addr_of(root), key);
        if (!authum_null_eq(member) && lh_web_json_node_get_sint(member, lh_addr_of(value)) &&
            lh_math_ge(value, lh_math_sub(-2147483647LL, 1)) && lh_math_le(value, 2147483647LL))
        {
            lh_ptr_deref(out) = lh_cast_static(authum_i32_t, value);
            found = authum_bool_true;
        }
    }
    lh_web_json_node_deinit(lh_addr_of(root));
    return found;
}

authum_i32_t
authum_json_object_get_i32(const authum_char_t *json, const authum_char_t *key,
                           authum_i32_t fallback)
{
    authum_i32_t v;

    if (authum_json_object_try_get_i32(json, key, lh_addr_of(v)))
    {
        return v;
    }
    return fallback;
}

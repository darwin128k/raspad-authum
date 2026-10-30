#include <authum/json.h>
#include <lh/char/map.h>
#include <lh/memory/std.h>

#include <lh/json.h>

/* Read @p json into @p root; the caller deinits @p root whatever this returns. */
static authum_bool_t
authum_json_read(const authum_char_t *json, lh_json_node_t *root)
{
    lh_json_reader_t reader;
    lh_str_view_t text;

    lh_json_node_init(root);
    if (authum_null_eq(json))
    {
        return authum_bool_false;
    }
    lh_str_view_init(lh_addr_of(text), json);
    lh_json_reader_init(lh_addr_of(reader), text);
    return lh_json_reader_read(lh_addr_of(reader), root) ? authum_bool_true : authum_bool_false;
}

/* The member @p key of the object @p root, or null. */
static const lh_json_node_t *
authum_json_object_find(const lh_json_node_t *root, const authum_char_t *key)
{
    lh_str_view_t name;

    if (authum_null_eq(key) || lh_math_eq(key[0], lh_char_map_nul))
    {
        return authum_null;
    }
    lh_str_view_init(lh_addr_of(name), key);
    return lh_json_node_get_member(root, name);
}

authum_bool_t
authum_json_object_copy_cstr(const authum_char_t *json, const authum_char_t *key,
                             authum_char_t *out, authum_usize_t out_size)
{
    lh_json_node_t root;
    const lh_json_node_t *member;
    authum_bool_t found = authum_bool_false;

    if (authum_null_eq(out) || lh_math_eq(out_size, 0u))
    {
        return authum_bool_false;
    }
    out[0] = lh_char_map_nul;

    if (authum_json_read(json, lh_addr_of(root)))
    {
        member = authum_json_object_find(lh_addr_of(root), key);
        if (!authum_null_eq(member) && lh_json_node_has_kind(member, lh_json_kind_string))
        {
            const lh_str_view_t value = lh_json_node_get_str(member);
            authum_usize_t n =
                lh_cast_static(authum_usize_t, lh_str_view_get_size(lh_addr_of(value)));

            /* Truncated to the buffer, as before; the text is already unescaped. */
            if (lh_math_gt(n, lh_math_sub(out_size, 1u)))
            {
                n = lh_math_sub(out_size, 1u);
            }
            if (lh_math_gt(n, 0u))
            {
                lh_memory_std_copy(out, lh_str_view_get_data(lh_addr_of(value)), n);
            }
            out[n] = lh_char_map_nul;
            found = authum_bool_true;
        }
    }
    lh_json_node_deinit(lh_addr_of(root));
    return found;
}

authum_bool_t
authum_json_object_try_get_i32(const authum_char_t *json, const authum_char_t *key,
                               authum_i32_t *out)
{
    lh_json_node_t root;
    const lh_json_node_t *member;
    lh_sllong_t value;
    authum_bool_t found = authum_bool_false;

    if (authum_null_eq(out))
    {
        return authum_bool_false;
    }
    if (authum_json_read(json, lh_addr_of(root)))
    {
        member = authum_json_object_find(lh_addr_of(root), key);
        if (!authum_null_eq(member) && lh_json_node_get_sint(member, lh_addr_of(value)) &&
            lh_math_ge(value, lh_math_sub(-2147483647LL, 1)) && lh_math_le(value, 2147483647LL))
        {
            lh_ptr_deref(out) = lh_cast_static(authum_i32_t, value);
            found = authum_bool_true;
        }
    }
    lh_json_node_deinit(lh_addr_of(root));
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

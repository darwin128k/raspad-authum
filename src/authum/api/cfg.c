/**
 * @file cfg.c
 * @brief Flat cfg reader — part of the `authum_client` static library.
 */

#include <authum/api/cfg/private.h>
#include <authum/memory.h>
#include <authum/unused.h>

/**
 * @brief Read the whole file into a block of our own.
 *
 * The file is read once, when it is opened: every getter scans the text from
 * its first line again, the way the old stream was rewound and read. A file
 * that cannot be opened or read leaves the config closed.
 */
authum_bool_t
authum_cfg_read_file(const lh_fs_path_t *path, authum_char_t **out_text, authum_usize_t *out_size)
{
    lh_str_t text;
    lh_str_init(lh_addr_of(text));
    if (!lh_os_fs_file_read_all_by_path(path, lh_addr_of(text)))
    {
        lh_str_deinit(lh_addr_of(text));
        return authum_bool_false;
    }
    lh_usize_t size = lh_str_get_size(lh_addr_of(text));
    authum_char_t *bytes =
        lh_cast_static(authum_char_t *, authum_memory_alloc(lh_math_add(size, 1u)));
    if (authum_null_eq(bytes))
    {
        lh_str_deinit(lh_addr_of(text));
        return authum_bool_false;
    }
    lh_memory_std_copy(bytes, lh_str_get_data(lh_addr_of(text)), size);
    bytes[size] = lh_char_map_nul;
    lh_str_deinit(lh_addr_of(text));
    *out_text = bytes;
    *out_size = size;
    return authum_bool_true;
}

void
authum_cfg_close(authum_cfg_t *self)
{
    if (authum_null_eq(self))
    {
        return;
    }
    if (!authum_null_eq(self->text))
    {
        authum_memory_free(self->text);
        self->text = authum_null;
    }
    self->size = 0u;
    self->line[0] = lh_char_map_nul;
}

authum_bool_t
authum_cfg_open_cstr(authum_cfg_t *self, const authum_char_t *path)
{
    if (authum_null_eq(self))
    {
        return authum_bool_false;
    }
    self->text = authum_null;
    self->size = 0u;
    self->line[0] = lh_char_map_nul;
    if (authum_null_eq(path) || lh_math_eq(path[0], lh_char_map_nul))
    {
        return authum_bool_false;
    }
    lh_fs_path_t file_path;
    lh_fs_path_init(lh_addr_of(file_path));
    lh_str_view_t view;
    lh_str_view_init(lh_addr_of(view), path);
    lh_fs_path_set(lh_addr_of(file_path), view, lh_os_system_fs_path_style_native());
    authum_bool_t ok =
        authum_cfg_read_file(lh_addr_of(file_path), lh_addr_of(self->text), lh_addr_of(self->size));
    lh_fs_path_deinit(lh_addr_of(file_path));
    return ok;
}

authum_bool_t
authum_cfg_open(authum_cfg_t *self, const authum_path_t *path)
{
    return authum_cfg_open_cstr(self, authum_path_get_cstr_or_null(path));
}

authum_bool_t
authum_cfg_open_text(authum_cfg_t *self, const authum_char_t *text, authum_usize_t size)
{
    authum_char_t *bytes;

    if (authum_null_eq(self))
    {
        return authum_bool_false;
    }
    self->text = authum_null;
    self->size = 0u;
    self->line[0] = lh_char_map_nul;
    if (authum_null_eq(text) && lh_math_gt(size, 0u))
    {
        return authum_bool_false;
    }
    bytes = lh_cast_static(authum_char_t *, authum_memory_alloc(lh_math_add(size, 1u)));
    if (authum_null_eq(bytes))
    {
        return authum_bool_false;
    }
    if (lh_math_gt(size, 0u))
    {
        lh_memory_std_copy(bytes, text, size);
    }
    bytes[size] = lh_char_map_nul;
    self->text = bytes;
    self->size = size;
    return authum_bool_true;
}

const authum_char_t *
authum_cfg_get_value(authum_cfg_t *self, const authum_char_t *key)
{
    if (authum_null_eq(self) || authum_null_eq(self->text) || authum_null_eq(key) ||
        lh_math_eq(key[0], lh_char_map_nul))
    {
        return authum_null;
    }
    lh_str_view_t text;
    lh_str_view_init_by_size(lh_addr_of(text), self->text, self->size);
    lh_str_view_t wanted;
    lh_str_view_init(lh_addr_of(wanted), key);
    /* A flat ini text: no headers needed, a `[section]` line is passed over. */
    lh_ini_parser_t parser;
    lh_ini_parser_init_flat(lh_addr_of(parser), text);
    lh_ini_entry_t entry;
    for (;;)
    {
        if (!lh_ini_parser_next(lh_addr_of(parser), lh_addr_of(entry)))
        {
            if (!lh_ini_parser_has_error(lh_addr_of(parser)))
            {
                return authum_null;
            }
            /* The cfg is lenient: a line that is not `key = value` is passed over. */
            lh_ini_parser_set_error(lh_addr_of(parser), lh_ini_error_none);
            continue;
        }
        lh_str_view_t name = lh_ini_entry_get_key(lh_addr_of(entry));
        if (!lh_ini_entry_has_kind(lh_addr_of(entry), lh_ini_entry_kind_pair) ||
            !lh_str_view_equals(lh_addr_of(name), lh_addr_of(wanted), lh_bool_true))
        {
            continue;
        }
        lh_str_view_t value = lh_ini_entry_get_value(lh_addr_of(entry));
        /* The value is handed out NUL-terminated, cut to the line buffer. */
        lh_usize_t size =
            lh_math_min(lh_str_view_get_size(lh_addr_of(value)),
                        lh_math_sub(lh_cast_static(lh_usize_t, sizeof(self->line)), 1u));
        lh_memory_std_copy(self->line, lh_str_view_get_data(lh_addr_of(value)), size);
        self->line[size] = lh_char_map_nul;
        return self->line;
    }
}

authum_bool_t
authum_cfg_get_bool(authum_cfg_t *self, const authum_char_t *key, authum_bool_t default_value)
{
    const authum_char_t *v;
    authum_bool_t out;

    v = authum_cfg_get_value(self, key);
    if (authum_null_eq(v) || !authum_cstr_parse_bool(v, lh_addr_of(out)))
    {
        return default_value;
    }
    return out;
}

authum_i32_t
authum_cfg_get_i32(authum_cfg_t *self, const authum_char_t *key, authum_i32_t default_value)
{
    const authum_char_t *v;
    authum_i32_t out;

    v = authum_cfg_get_value(self, key);
    if (authum_null_eq(v) || !authum_cstr_parse_i32(v, lh_addr_of(out)))
    {
        return default_value;
    }
    return out;
}

authum_u32_t
authum_cfg_get_u32(authum_cfg_t *self, const authum_char_t *key, authum_u32_t default_value)
{
    const authum_char_t *v;
    authum_u32_t out;

    v = authum_cfg_get_value(self, key);
    if (authum_null_eq(v) || !authum_cstr_parse_u32(v, lh_addr_of(out)))
    {
        return default_value;
    }
    return out;
}

authum_bool_t
authum_cfg_copy_cstr(authum_cfg_t *self, const authum_char_t *key, authum_char_t *buf,
                     authum_usize_t buf_size, const authum_char_t *default_value)
{
    const authum_char_t *v;

    v = authum_cfg_get_value(self, key);
    if (authum_null_eq(v))
    {
        authum_unused(authum_cstr_copy(buf, buf_size, default_value));
        return authum_bool_false;
    }
    authum_unused(authum_cstr_copy(buf, buf_size, v));
    return authum_bool_true;
}

authum_bool_t
authum_cfg_apply_bool(authum_cfg_t *self, const authum_char_t *key, authum_bool_t *out)
{
    const authum_char_t *v;
    authum_bool_t parsed;

    if (authum_null_eq(out))
    {
        return authum_bool_false;
    }
    v = authum_cfg_get_value(self, key);
    if (authum_null_eq(v) || !authum_cstr_parse_bool(v, lh_addr_of(parsed)))
    {
        return authum_bool_false;
    }
    lh_ptr_deref(out) = parsed;
    return authum_bool_true;
}

authum_bool_t
authum_cfg_apply_i32(authum_cfg_t *self, const authum_char_t *key, authum_i32_t *out)
{
    const authum_char_t *v;
    authum_i32_t parsed;

    if (authum_null_eq(out))
    {
        return authum_bool_false;
    }
    v = authum_cfg_get_value(self, key);
    if (authum_null_eq(v) || !authum_cstr_parse_i32(v, lh_addr_of(parsed)))
    {
        return authum_bool_false;
    }
    lh_ptr_deref(out) = parsed;
    return authum_bool_true;
}

authum_bool_t
authum_cfg_apply_u32(authum_cfg_t *self, const authum_char_t *key, authum_u32_t *out)
{
    const authum_char_t *v;
    authum_u32_t parsed;

    if (authum_null_eq(out))
    {
        return authum_bool_false;
    }
    v = authum_cfg_get_value(self, key);
    if (authum_null_eq(v) || !authum_cstr_parse_u32(v, lh_addr_of(parsed)))
    {
        return authum_bool_false;
    }
    lh_ptr_deref(out) = parsed;
    return authum_bool_true;
}

authum_bool_t
authum_cfg_apply_cstr(authum_cfg_t *self, const authum_char_t *key, authum_char_t *buf,
                      authum_usize_t buf_size)
{
    const authum_char_t *v;

    v = authum_cfg_get_value(self, key);
    if (authum_null_eq(v))
    {
        return authum_bool_false;
    }
    return authum_cstr_copy(buf, buf_size, v);
}

authum_bool_t
authum_cfg_apply_item(authum_cfg_t *self, const authum_cfg_item_t *item)
{
    if (authum_null_eq(item) || authum_null_eq(item->ptr) || authum_null_eq(item->key))
    {
        return authum_bool_false;
    }
    switch (item->kind)
    {
    case authum_cfg_kind_bool:
        return authum_cfg_apply_bool(self, item->key,
                                     lh_cast_reinterpret(authum_bool_t *, item->ptr));
    case authum_cfg_kind_i32:
        return authum_cfg_apply_i32(self, item->key,
                                    lh_cast_reinterpret(authum_i32_t *, item->ptr));
    case authum_cfg_kind_u32:
        return authum_cfg_apply_u32(self, item->key,
                                    lh_cast_reinterpret(authum_u32_t *, item->ptr));
    case authum_cfg_kind_cstr:
        return authum_cfg_apply_cstr(self, item->key,
                                     lh_cast_reinterpret(authum_char_t *, item->ptr), item->size);
    default:
        return authum_bool_false;
    }
}

void
authum_cfg_apply_items(authum_cfg_t *self, const authum_cfg_item_t *items, authum_usize_t count)
{
    authum_usize_t i;

    if (authum_null_eq(items))
    {
        return;
    }
    for (i = 0u; lh_math_lt(i, count); i = lh_math_add_one(i))
    {
        authum_unused(authum_cfg_apply_item(self, lh_addr_of(items[i])));
    }
}

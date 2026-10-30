/**
 * @file cfg.c
 * @brief Flat cfg reader — part of the `authum_client` static library.
 */

#include <authum/cfg.h>
#include <authum/memory.h>
#include <authum/unused.h>
#include <lh/cast/const.h>
#include <lh/char/map.h>
#include <lh/memory/std.h>
#include <lh/os/fs/file.h>
#include <lh/os/system/fs/path.h>
#include <lh/str.h>
#include <lh/str/line/reader.h>
#include <lh/util/str/ptr.h>

/** The bytes one read of the config file brings in. */
enum
{
    authum_cfg_read_chunk = 512
};

/**
 * @brief Read the whole file into a block of our own.
 *
 * The file is read once, when it is opened: every getter scans the text from
 * its first line again, the way the old stream was rewound and read. A file
 * that cannot be opened or read leaves the config closed.
 */
static authum_bool_t
authum_cfg_read_file(const lh_fs_path_t *path, authum_char_t **out_text, authum_usize_t *out_size)
{
    lh_os_fs_file_t file;
    lh_str_t text;
    lh_char_t buf[authum_cfg_read_chunk];
    lh_ssize_t n;
    authum_bool_t ok;
    authum_char_t *bytes;
    authum_usize_t size;

    lh_str_init(lh_addr_of(text));
    lh_os_fs_file_init(lh_addr_of(file));
    if (!lh_os_fs_file_open(lh_addr_of(file), path, lh_fs_file_mode_read))
    {
        lh_os_fs_file_deinit(lh_addr_of(file));
        lh_str_deinit(lh_addr_of(text));
        return authum_bool_false;
    }
    ok = authum_bool_true;
    for (;;)
    {
        n = lh_os_fs_file_read(lh_addr_of(file), buf, sizeof(buf));
        if (lh_math_is_negative(n))
        {
            ok = authum_bool_false;
            break;
        }
        if (lh_math_is_zero(n))
        {
            break;
        }
        lh_str_append(lh_addr_of(text), buf, lh_cast_static(lh_usize_t, n));
    }
    lh_os_fs_file_deinit(lh_addr_of(file));
    if (!ok)
    {
        lh_str_deinit(lh_addr_of(text));
        return authum_bool_false;
    }
    size = lh_str_get_size(lh_addr_of(text));
    bytes = lh_cast_static(authum_char_t *, authum_memory_alloc(lh_math_add(size, 1u)));
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
    lh_fs_path_t file_path;
    lh_str_view_t view;
    authum_bool_t ok;

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
    lh_fs_path_init(lh_addr_of(file_path));
    lh_str_view_init(lh_addr_of(view), path);
    lh_fs_path_set(lh_addr_of(file_path), view, lh_os_system_fs_path_style_native());
    ok = authum_cfg_read_file(lh_addr_of(file_path), lh_addr_of(self->text), lh_addr_of(self->size));
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

authum_char_t *
authum_cfg_skip_spaces(authum_char_t *s)
{
    while (!authum_null_eq(s) && (lh_math_eq(lh_ptr_deref(s), lh_char_map_space) ||
                                  lh_math_eq(lh_ptr_deref(s), lh_char_map_ht)))
    {
        s = lh_math_add_one(s);
    }
    return s;
}

void
authum_cfg_rtrim(authum_char_t *s)
{
    authum_usize_t n;

    if (authum_null_eq(s))
    {
        return;
    }
    n = lh_cast_static(authum_usize_t, lh_str_ptr_len(s));
    while (lh_math_gt(n, 0u) && (lh_math_eq(s[lh_math_sub(n, 1u)], lh_char_map_lf) ||
                                 lh_math_eq(s[lh_math_sub(n, 1u)], lh_char_map_cr) ||
                                 lh_math_eq(s[lh_math_sub(n, 1u)], lh_char_map_space) ||
                                 lh_math_eq(s[lh_math_sub(n, 1u)], lh_char_map_ht)))
    {
        s[--n] = lh_char_map_nul;
    }
}

/**
 * @brief The next line of the config, the way the old stream read it.
 *
 * At most authum_cfg_line_max - 1 bytes; a longer line continues in the next
 * call. @p pos is the offset of the line to read.
 */
static authum_bool_t
authum_cfg_next_line(authum_cfg_t *self, lh_str_line_reader_t *reader, lh_usize_t *pos)
{
    lh_str_view_t line;
    lh_usize_t limit;
    lh_usize_t chunk;

    limit = lh_math_sub(lh_cast_static(lh_usize_t, sizeof(self->line)), 1u);
    lh_str_line_reader_set_pos(reader, *pos);
    if (!lh_str_line_reader_next(reader, lh_addr_of(line)))
    {
        return authum_bool_false;
    }
    chunk = lh_str_view_get_size(lh_addr_of(line));
    if (chunk > limit)
    {
        chunk = limit;
    }
    lh_memory_std_copy(self->line, lh_str_view_get_data(lh_addr_of(line)), chunk);
    self->line[chunk] = lh_char_map_nul;
    *pos = lh_math_add(*pos, chunk);
    return authum_bool_true;
}

const authum_char_t *
authum_cfg_get_value(authum_cfg_t *self, const authum_char_t *key)
{
    authum_char_t *p;
    authum_char_t *eq;
    authum_char_t *key_end;
    authum_char_t *value;
    lh_str_view_t text;
    lh_str_line_reader_t reader;
    lh_usize_t pos;

    if (authum_null_eq(self) || authum_null_eq(self->text) || authum_null_eq(key) ||
        lh_math_eq(key[0], lh_char_map_nul))
    {
        return authum_null;
    }

    lh_str_view_init_by_size(lh_addr_of(text), self->text, self->size);
    lh_str_line_reader_init(lh_addr_of(reader), text);
    pos = 0u;
    while (authum_cfg_next_line(self, lh_addr_of(reader), lh_addr_of(pos)))
    {
        p = authum_cfg_skip_spaces(self->line);
        authum_cfg_rtrim(p);
        if (lh_math_eq(p[0], lh_char_map_nul) || lh_math_eq(p[0], lh_char_map_hash) ||
            lh_math_eq(p[0], lh_char_map_semicolon))
        {
            continue;
        }

        eq = lh_cast_const(authum_char_t *,
                           lh_str_ptr_find_of_char(p, lh_str_ptr_len(p), lh_char_map_equals));
        if (authum_null_eq(eq))
        {
            continue;
        }
        lh_ptr_deref(eq) = lh_char_map_nul;
        key_end = eq;
        while (lh_math_gt(key_end, p) && (lh_math_eq(key_end[-1], lh_char_map_space) ||
                                          lh_math_eq(key_end[-1], lh_char_map_ht)))
        {
            key_end = lh_math_sub_one(key_end);
            lh_ptr_deref(key_end) = lh_char_map_nul;
        }
        if (lh_math_eq(p[0], lh_char_map_nul) || !authum_cstr_is_equal(p, key, authum_bool_true))
        {
            continue;
        }

        value = authum_cfg_skip_spaces(lh_math_add(eq, 1));
        if (lh_math_ne(value, self->line))
        {
            authum_usize_t n = lh_cast_static(authum_usize_t, lh_str_ptr_len(value));
            lh_memory_std_move(self->line, value, lh_math_add(n, 1u));
        }
        return self->line;
    }
    return authum_null;
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

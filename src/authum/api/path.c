#include <authum/path.h>
#include <lh/char/map.h>
#include <lh/memory/std.h>

#include <lh/os/fs/dir.h>
#include <lh/os/system/fs/path.h>

void
authum_path_clear(authum_path_t *self)
{
    if (authum_null_eq(self))
    {
        return;
    }
    self->data[0] = lh_char_map_nul;
    self->size = 0u;
}

authum_bool_t
authum_path_is_empty(const authum_path_t *self)
{
    return (authum_null_eq(self) || lh_math_eq(self->size, 0u)) ? authum_bool_true
                                                                : authum_bool_false;
}

const authum_char_t *
authum_path_get_cstr(const authum_path_t *self)
{
    if (authum_null_eq(self) || lh_math_eq(self->size, 0u))
    {
        return "";
    }
    return self->data;
}

const authum_char_t *
authum_path_get_cstr_or_null(const authum_path_t *self)
{
    if (authum_null_eq(self) || lh_math_eq(self->size, 0u))
    {
        return authum_null;
    }
    return self->data;
}

authum_bool_t
authum_path_set_cstr(authum_path_t *self, const authum_char_t *text)
{
    authum_usize_t n;

    if (authum_null_eq(self))
    {
        return authum_bool_false;
    }
    if (authum_null_eq(text) || lh_math_eq(text[0], lh_char_map_nul))
    {
        authum_path_clear(self);
        return authum_bool_true;
    }
    n = lh_cast_static(authum_usize_t, strlen(text));
    if (lh_math_ge(n, lh_cast_static(authum_usize_t, authum_path_capacity)))
    {
        authum_path_clear(self);
        return authum_bool_false;
    }
    lh_memory_std_copy(self->data, text, n);
    self->data[n] = lh_char_map_nul;
    self->size = n;
    return authum_bool_true;
}

authum_bool_t
authum_path_assign(authum_path_t *self, const authum_path_t *other)
{
    if (authum_null_eq(self))
    {
        return authum_bool_false;
    }
    if (authum_null_eq(other) || lh_math_eq(other->size, 0u))
    {
        authum_path_clear(self);
        return authum_bool_true;
    }
    if (lh_math_ge(other->size, lh_cast_static(authum_usize_t, authum_path_capacity)))
    {
        authum_path_clear(self);
        return authum_bool_false;
    }
    lh_memory_std_copy(self->data, other->data, other->size);
    self->data[other->size] = lh_char_map_nul;
    self->size = other->size;
    return authum_bool_true;
}

authum_bool_t
authum_path_has_trailing_sep(const authum_path_t *self)
{
    authum_char_t c;

    if (authum_path_is_empty(self))
    {
        return authum_bool_false;
    }
    c = self->data[lh_math_sub(self->size, 1u)];
    return (lh_math_eq(c, lh_char_map_slash) || lh_math_eq(c, lh_char_map_backslash))
               ? authum_bool_true
               : authum_bool_false;
}

authum_bool_t
authum_path_join(authum_path_t *self, const authum_path_t *dir, const authum_char_t *leaf)
{
    authum_usize_t na;
    authum_usize_t nb;
    authum_usize_t need;
    authum_bool_t slash;

    if (authum_null_eq(self) || authum_null_eq(dir) || authum_null_eq(leaf) ||
        lh_math_eq(leaf[0], lh_char_map_nul))
    {
        return authum_bool_false;
    }
    if (authum_path_is_empty(dir))
    {
        return authum_path_set_cstr(self, leaf);
    }

    na = dir->size;
    nb = lh_cast_static(authum_usize_t, strlen(leaf));
    slash = authum_path_has_trailing_sep(dir);
    need = lh_math_add(lh_math_add(na, (slash ? 0u : 1u)), nb);
    if (lh_math_ge(need, lh_cast_static(authum_usize_t, authum_path_capacity)))
    {
        authum_path_clear(self);
        return authum_bool_false;
    }

    if (lh_math_ne(self, dir))
    {
        lh_memory_std_copy(self->data, dir->data, na);
    }
    if (!slash)
    {
        self->data[na++] = authum_path_sep;
    }
    lh_memory_std_copy(lh_math_add(self->data, na), leaf, nb);
    self->data[lh_math_add(na, nb)] = lh_char_map_nul;
    self->size = lh_math_add(na, nb);
    return authum_bool_true;
}

authum_bool_t
authum_path_assign_parent(authum_path_t *self, const authum_path_t *path)
{
    authum_usize_t end;
    authum_usize_t cut;

    if (authum_null_eq(self) || authum_path_is_empty(path))
    {
        if (!authum_null_eq(self))
        {
            authum_path_clear(self);
        }
        return authum_bool_false;
    }

    end = path->size;
    while (lh_math_gt(end, 0u) &&
           (lh_math_eq(path->data[lh_math_sub(end, 1u)], lh_char_map_slash) ||
            lh_math_eq(path->data[lh_math_sub(end, 1u)], lh_char_map_backslash)))
    {
        end = lh_math_sub_one(end);
    }
    if (lh_math_eq(end, 0u))
    {
        authum_path_clear(self);
        return authum_bool_false;
    }

    cut = end;
    while (lh_math_gt(cut, 0u) && lh_math_ne(path->data[lh_math_sub(cut, 1u)], lh_char_map_slash) &&
           lh_math_ne(path->data[lh_math_sub(cut, 1u)], lh_char_map_backslash))
    {
        cut = lh_math_sub_one(cut);
    }
    if (lh_math_eq(cut, 0u))
    {
        authum_path_clear(self);
        return authum_bool_false;
    }

    /* cut is index after the separator that ends the parent. */
    if (lh_math_eq(cut, 1u))
    {
        /* POSIX root `/…`. */
    }
    else if (lh_math_eq(cut, 3u) && lh_math_eq(path->data[1], lh_char_map_colon) &&
             (lh_math_eq(path->data[2], lh_char_map_slash) ||
              lh_math_eq(path->data[2], lh_char_map_backslash)))
    {
        /* Drive root `C:/…`. */
    }
    else
    {
        cut = lh_math_sub_one(cut); /* drop the trailing separator for normal parents */
        while (lh_math_gt(cut, 0u) &&
               (lh_math_eq(path->data[lh_math_sub(cut, 1u)], lh_char_map_slash) ||
                lh_math_eq(path->data[lh_math_sub(cut, 1u)], lh_char_map_backslash)))
        {
            cut = lh_math_sub_one(cut);
        }
        if (lh_math_eq(cut, 0u))
        {
            cut = 1u;
        }
        else if (lh_math_eq(cut, 2u) && lh_math_eq(path->data[1], lh_char_map_colon))
        {
            cut = 3u;
            if (lh_math_ge(path->size, 3u) && lh_math_ne(path->data[2], lh_char_map_slash) &&
                lh_math_ne(path->data[2], lh_char_map_backslash))
            {
                cut = 2u;
            }
        }
    }

    if (lh_math_ge(cut, lh_cast_static(authum_usize_t, authum_path_capacity)))
    {
        authum_path_clear(self);
        return authum_bool_false;
    }
    lh_memory_std_copy(self->data, path->data, cut);
    self->data[cut] = lh_char_map_nul;
    self->size = cut;
    return authum_bool_true;
}

authum_bool_t
authum_path_ensure_dir(const authum_path_t *self)
{
    lh_fs_path_t dir;
    lh_str_view_t text;
    lh_bool_t ok;

    if (authum_path_is_empty(self))
    {
        return authum_bool_false;
    }
    lh_fs_path_init(lh_addr_of(dir));
    lh_str_view_init(lh_addr_of(text), authum_path_get_cstr(self));
    lh_fs_path_set(lh_addr_of(dir), text, lh_os_system_fs_path_style_native());
    ok = lh_os_fs_dir_create(lh_addr_of(dir));
    lh_fs_path_deinit(lh_addr_of(dir));
    return ok ? authum_bool_true : authum_bool_false;
}

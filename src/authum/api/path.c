#include <authum/path.h>

#include "os/os.h"

void
authum_path_clear(authum_path_t *self)
{
    if (authum_null_eq(self))
    {
        return;
    }
    self->data[0] = '\0';
    self->size = 0u;
}

authum_bool_t
authum_path_is_empty(const authum_path_t *self)
{
    return (authum_null_eq(self) || self->size == 0u) ? authum_bool_true : authum_bool_false;
}

const authum_char_t *
authum_path_as_cstr(const authum_path_t *self)
{
    if (authum_null_eq(self) || self->size == 0u)
    {
        return "";
    }
    return self->data;
}

const authum_char_t *
authum_path_as_cstr_or_null(const authum_path_t *self)
{
    if (authum_null_eq(self) || self->size == 0u)
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
    if (authum_null_eq(text) || text[0] == '\0')
    {
        authum_path_clear(self);
        return authum_bool_true;
    }
    n = (authum_usize_t)strlen(text);
    if (n >= (authum_usize_t)authum_path_capacity)
    {
        authum_path_clear(self);
        return authum_bool_false;
    }
    memcpy(self->data, text, n);
    self->data[n] = '\0';
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
    if (authum_null_eq(other) || other->size == 0u)
    {
        authum_path_clear(self);
        return authum_bool_true;
    }
    if (other->size >= (authum_usize_t)authum_path_capacity)
    {
        authum_path_clear(self);
        return authum_bool_false;
    }
    memcpy(self->data, other->data, other->size);
    self->data[other->size] = '\0';
    self->size = other->size;
    return authum_bool_true;
}

authum_bool_t
authum_path_ends_with_sep(const authum_path_t *self)
{
    authum_char_t c;

    if (authum_path_is_empty(self))
    {
        return authum_bool_false;
    }
    c = self->data[self->size - 1u];
    return (c == '/' || c == '\\') ? authum_bool_true : authum_bool_false;
}

authum_bool_t
authum_path_join(authum_path_t *self, const authum_path_t *dir, const authum_char_t *leaf)
{
    authum_usize_t na;
    authum_usize_t nb;
    authum_usize_t need;
    authum_bool_t slash;

    if (authum_null_eq(self) || authum_null_eq(dir) || authum_null_eq(leaf) || leaf[0] == '\0')
    {
        return authum_bool_false;
    }
    if (authum_path_is_empty(dir))
    {
        return authum_path_set_cstr(self, leaf);
    }

    na = dir->size;
    nb = (authum_usize_t)strlen(leaf);
    slash = authum_path_ends_with_sep(dir);
    need = na + (slash ? 0u : 1u) + nb;
    if (need >= (authum_usize_t)authum_path_capacity)
    {
        authum_path_clear(self);
        return authum_bool_false;
    }

    if (self != dir)
    {
        memcpy(self->data, dir->data, na);
    }
    if (!slash)
    {
        self->data[na++] = authum_path_sep;
    }
    memcpy(self->data + na, leaf, nb);
    self->data[na + nb] = '\0';
    self->size = na + nb;
    return authum_bool_true;
}

authum_bool_t
authum_path_parent(authum_path_t *self, const authum_path_t *path)
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
    while (end > 0u && (path->data[end - 1u] == '/' || path->data[end - 1u] == '\\'))
    {
        --end;
    }
    if (end == 0u)
    {
        authum_path_clear(self);
        return authum_bool_false;
    }

    cut = end;
    while (cut > 0u && path->data[cut - 1u] != '/' && path->data[cut - 1u] != '\\')
    {
        --cut;
    }
    if (cut == 0u)
    {
        authum_path_clear(self);
        return authum_bool_false;
    }

    /* cut is index after the separator that ends the parent. */
    if (cut == 1u)
    {
        /* POSIX root `/…`. */
    }
    else if (cut == 3u && path->data[1] == ':' && (path->data[2] == '/' || path->data[2] == '\\'))
    {
        /* Drive root `C:/…`. */
    }
    else
    {
        --cut; /* drop the trailing separator for normal parents */
        while (cut > 0u && (path->data[cut - 1u] == '/' || path->data[cut - 1u] == '\\'))
        {
            --cut;
        }
        if (cut == 0u)
        {
            cut = 1u;
        }
        else if (cut == 2u && path->data[1] == ':')
        {
            cut = 3u;
            if (path->size >= 3u && path->data[2] != '/' && path->data[2] != '\\')
            {
                cut = 2u;
            }
        }
    }

    if (cut >= (authum_usize_t)authum_path_capacity)
    {
        authum_path_clear(self);
        return authum_bool_false;
    }
    memcpy(self->data, path->data, cut);
    self->data[cut] = '\0';
    self->size = cut;
    return authum_bool_true;
}

authum_bool_t
authum_path_ensure_dir(const authum_path_t *self)
{
    return authum_path_is_empty(self) ? authum_bool_false
                                      : authum_os_dir_ensure(authum_path_as_cstr(self));
}

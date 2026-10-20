/**
 * @file cfg.c
 * @brief Flat cfg reader — part of the `authum_client` static library.
 */

#include <authum/cfg.h>
#include <authum/cstr.h>

#include <string.h>

void
authum_cfg_close(authum_cfg_t *self)
{
    if (authum_null_eq(self)) {
        return;
    }
    if (!authum_null_eq(self->file)) {
        fclose(self->file);
        self->file = authum_null;
    }
    self->line[0] = '\0';
}

authum_bool_t
authum_cfg_open_cstr(authum_cfg_t *self, const authum_char_t *path)
{
    if (authum_null_eq(self)) {
        return authum_bool_false;
    }
    self->file = authum_null;
    self->line[0] = '\0';
    if (authum_null_eq(path) || path[0] == '\0') {
        return authum_bool_false;
    }
    self->file = fopen(path, "r");
    return (!authum_null_eq(self->file)) ? authum_bool_true : authum_bool_false;
}

authum_bool_t
authum_cfg_open(authum_cfg_t *self, const authum_path_t *path)
{
    return authum_cfg_open_cstr(self, authum_path_as_cstr_or_null(path));
}

authum_bool_t
authum_cfg_adopt(authum_cfg_t *self, FILE *file)
{
    if (authum_null_eq(self) || authum_null_eq(file)) {
        return authum_bool_false;
    }
    self->file = file;
    self->line[0] = '\0';
    return authum_bool_true;
}

static authum_char_t *
authum_cfg_skip_spaces(authum_char_t *s)
{
    while (!authum_null_eq(s) && (*s == ' ' || *s == '\t')) {
        ++s;
    }
    return s;
}

static void
authum_cfg_rtrim(authum_char_t *s)
{
    authum_usize_t n;

    if (authum_null_eq(s)) {
        return;
    }
    n = (authum_usize_t)strlen(s);
    while (n > 0u &&
           (s[n - 1u] == '\n' || s[n - 1u] == '\r' || s[n - 1u] == ' ' || s[n - 1u] == '\t')) {
        s[--n] = '\0';
    }
}

const authum_char_t *
authum_cfg_get_value(authum_cfg_t *self, const authum_char_t *key)
{
    authum_char_t *p;
    authum_char_t *eq;
    authum_char_t *key_end;
    authum_char_t *value;

    if (authum_null_eq(self) || authum_null_eq(self->file) || authum_null_eq(key) ||
        key[0] == '\0') {
        return authum_null;
    }

    rewind(self->file);
    while (!authum_null_eq(fgets(self->line, (int)sizeof(self->line), self->file))) {
        p = authum_cfg_skip_spaces(self->line);
        authum_cfg_rtrim(p);
        if (p[0] == '\0' || p[0] == '#' || p[0] == ';') {
            continue;
        }

        eq = strchr(p, '=');
        if (authum_null_eq(eq)) {
            continue;
        }
        *eq = '\0';
        key_end = eq;
        while (key_end > p && (key_end[-1] == ' ' || key_end[-1] == '\t')) {
            --key_end;
            *key_end = '\0';
        }
        if (p[0] == '\0' || !authum_cstr_eq(p, key, authum_bool_true)) {
            continue;
        }

        value = authum_cfg_skip_spaces(eq + 1);
        if (value != self->line) {
            authum_usize_t n = (authum_usize_t)strlen(value);
            memmove(self->line, value, n + 1u);
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
    if (authum_null_eq(v) || !authum_cstr_parse_bool(v, &out)) {
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
    if (authum_null_eq(v) || !authum_cstr_parse_i32(v, &out)) {
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
    if (authum_null_eq(v) || !authum_cstr_parse_u32(v, &out)) {
        return default_value;
    }
    return out;
}

authum_bool_t
authum_cfg_get_cstr(authum_cfg_t *self, const authum_char_t *key, authum_char_t *buf,
                    authum_usize_t buf_size, const authum_char_t *default_value)
{
    const authum_char_t *v;

    v = authum_cfg_get_value(self, key);
    if (authum_null_eq(v)) {
        (void)authum_cstr_copy(buf, buf_size, default_value);
        return authum_bool_false;
    }
    (void)authum_cstr_copy(buf, buf_size, v);
    return authum_bool_true;
}

authum_bool_t
authum_cfg_apply_bool(authum_cfg_t *self, const authum_char_t *key, authum_bool_t *out)
{
    const authum_char_t *v;
    authum_bool_t parsed;

    if (authum_null_eq(out)) {
        return authum_bool_false;
    }
    v = authum_cfg_get_value(self, key);
    if (authum_null_eq(v) || !authum_cstr_parse_bool(v, &parsed)) {
        return authum_bool_false;
    }
    *out = parsed;
    return authum_bool_true;
}

authum_bool_t
authum_cfg_apply_i32(authum_cfg_t *self, const authum_char_t *key, authum_i32_t *out)
{
    const authum_char_t *v;
    authum_i32_t parsed;

    if (authum_null_eq(out)) {
        return authum_bool_false;
    }
    v = authum_cfg_get_value(self, key);
    if (authum_null_eq(v) || !authum_cstr_parse_i32(v, &parsed)) {
        return authum_bool_false;
    }
    *out = parsed;
    return authum_bool_true;
}

authum_bool_t
authum_cfg_apply_u32(authum_cfg_t *self, const authum_char_t *key, authum_u32_t *out)
{
    const authum_char_t *v;
    authum_u32_t parsed;

    if (authum_null_eq(out)) {
        return authum_bool_false;
    }
    v = authum_cfg_get_value(self, key);
    if (authum_null_eq(v) || !authum_cstr_parse_u32(v, &parsed)) {
        return authum_bool_false;
    }
    *out = parsed;
    return authum_bool_true;
}

authum_bool_t
authum_cfg_apply_cstr(authum_cfg_t *self, const authum_char_t *key, authum_char_t *buf,
                      authum_usize_t buf_size)
{
    const authum_char_t *v;

    v = authum_cfg_get_value(self, key);
    if (authum_null_eq(v)) {
        return authum_bool_false;
    }
    return authum_cstr_copy(buf, buf_size, v);
}

authum_bool_t
authum_cfg_apply_item(authum_cfg_t *self, const authum_cfg_item_t *item)
{
    if (authum_null_eq(item) || authum_null_eq(item->ptr) || authum_null_eq(item->key)) {
        return authum_bool_false;
    }
    switch (item->kind) {
    case authum_cfg_kind_bool:
        return authum_cfg_apply_bool(self, item->key, (authum_bool_t *)item->ptr);
    case authum_cfg_kind_i32:
        return authum_cfg_apply_i32(self, item->key, (authum_i32_t *)item->ptr);
    case authum_cfg_kind_u32:
        return authum_cfg_apply_u32(self, item->key, (authum_u32_t *)item->ptr);
    case authum_cfg_kind_cstr:
        return authum_cfg_apply_cstr(self, item->key, (authum_char_t *)item->ptr, item->size);
    default:
        return authum_bool_false;
    }
}

void
authum_cfg_apply_items(authum_cfg_t *self, const authum_cfg_item_t *items, authum_usize_t count)
{
    authum_usize_t i;

    if (authum_null_eq(items)) {
        return;
    }
    for (i = 0u; i < count; ++i) {
        (void)authum_cfg_apply_item(self, &items[i]);
    }
}

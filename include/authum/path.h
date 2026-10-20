/**
 * @file path.h
 * @brief Module-owned filesystem path (::authum_path_t) — no lh.
 *
 * Fixed-capacity, NUL-terminated text. Prefer ready-made directories from
 * ::authum_env_t; use ::authum_path_join only to append a leaf
 * (`qproto.cfg`, `sprites/foo.spr`, …).
 *
 * Host-filled env paths use the platform-native form (via lh inside
 * `authum_mm`). Module joins always insert `/` (::authum_path_sep); `/`
 * still works with `fopen` on Windows HLDS.
 */

#ifndef AUTHUM_PATH_H
#define AUTHUM_PATH_H

#include <authum/bool.h>
#include <authum/char.h>
#include <authum/cstr.h>
#include <authum/null.h>
#include <authum/path/capacity.h>
#include <authum/path/sep.h>
#include <authum/size.h>

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct authum_path
 * @brief Absolute or relative path text of at most ::authum_path_capacity - 1.
 */
struct authum_path
{
    authum_char_t data[authum_path_capacity];
    /** Payload length without the trailing NUL. */
    authum_usize_t size;
};
typedef struct authum_path authum_path_t;

static inline void
authum_path_clear(authum_path_t *self)
{
    if (authum_null_eq(self)) {
        return;
    }
    self->data[0] = '\0';
    self->size = 0u;
}

static inline authum_bool_t
authum_path_is_empty(const authum_path_t *self)
{
    return (authum_null_eq(self) || self->size == 0u) ? authum_bool_true : authum_bool_false;
}

/**
 * @brief C string view of @p self. Never null — empty path yields `""`.
 */
static inline const authum_char_t *
authum_path_as_cstr(const authum_path_t *self)
{
    if (authum_null_eq(self) || self->size == 0u) {
        return "";
    }
    return self->data;
}

/**
 * @brief Like ::authum_path_as_cstr, but null when @p self is null/empty.
 */
static inline const authum_char_t *
authum_path_as_cstr_or_null(const authum_path_t *self)
{
    if (authum_null_eq(self) || self->size == 0u) {
        return authum_null;
    }
    return self->data;
}

static inline authum_bool_t
authum_path_set_cstr(authum_path_t *self, const authum_char_t *text)
{
    authum_usize_t n;

    if (authum_null_eq(self)) {
        return authum_bool_false;
    }
    if (authum_null_eq(text) || text[0] == '\0') {
        authum_path_clear(self);
        return authum_bool_true;
    }
    n = (authum_usize_t)strlen(text);
    if (n >= (authum_usize_t)authum_path_capacity) {
        authum_path_clear(self);
        return authum_bool_false;
    }
    memcpy(self->data, text, n);
    self->data[n] = '\0';
    self->size = n;
    return authum_bool_true;
}

static inline authum_bool_t
authum_path_assign(authum_path_t *self, const authum_path_t *other)
{
    if (authum_null_eq(self)) {
        return authum_bool_false;
    }
    if (authum_null_eq(other) || other->size == 0u) {
        authum_path_clear(self);
        return authum_bool_true;
    }
    if (other->size >= (authum_usize_t)authum_path_capacity) {
        authum_path_clear(self);
        return authum_bool_false;
    }
    memcpy(self->data, other->data, other->size);
    self->data[other->size] = '\0';
    self->size = other->size;
    return authum_bool_true;
}

static inline authum_bool_t
authum_path_ends_with_sep(const authum_path_t *self)
{
    authum_char_t c;

    if (authum_path_is_empty(self)) {
        return authum_bool_false;
    }
    c = self->data[self->size - 1u];
    return (c == '/' || c == '\\') ? authum_bool_true : authum_bool_false;
}

/**
 * @brief Write @p dir + `/` + @p leaf into @p self (NUL-terminated).
 *
 * @p leaf may contain further `/` segments (`sprites/foo.spr`). Skips a
 * duplicate separator when @p dir already ends with `/` or `\\`.
 */
static inline authum_bool_t
authum_path_join(authum_path_t *self, const authum_path_t *dir, const authum_char_t *leaf)
{
    authum_usize_t na;
    authum_usize_t nb;
    authum_usize_t need;
    authum_bool_t slash;

    if (authum_null_eq(self) || authum_null_eq(dir) || authum_null_eq(leaf) || leaf[0] == '\0') {
        return authum_bool_false;
    }
    if (authum_path_is_empty(dir)) {
        return authum_path_set_cstr(self, leaf);
    }

    na = dir->size;
    nb = (authum_usize_t)strlen(leaf);
    slash = authum_path_ends_with_sep(dir);
    need = na + (slash ? 0u : 1u) + nb;
    if (need >= (authum_usize_t)authum_path_capacity) {
        authum_path_clear(self);
        return authum_bool_false;
    }

    if (self != dir) {
        memcpy(self->data, dir->data, na);
    }
    if (!slash) {
        self->data[na++] = authum_path_sep;
    }
    memcpy(self->data + na, leaf, nb);
    self->data[na + nb] = '\0';
    self->size = na + nb;
    return authum_bool_true;
}

/**
 * @brief Drop the last segment of @p path into @p self.
 *
 * Treats both `/` and `\\` as separators. Examples: `a/b/c` → `a/b`,
 * `/a` → `/`, `C:/a` → `C:/`. Fails when there is no parent segment.
 */
static inline authum_bool_t
authum_path_parent(authum_path_t *self, const authum_path_t *path)
{
    authum_usize_t end;
    authum_usize_t cut;

    if (authum_null_eq(self) || authum_path_is_empty(path)) {
        if (!authum_null_eq(self)) {
            authum_path_clear(self);
        }
        return authum_bool_false;
    }

    end = path->size;
    while (end > 0u && (path->data[end - 1u] == '/' || path->data[end - 1u] == '\\')) {
        --end;
    }
    if (end == 0u) {
        authum_path_clear(self);
        return authum_bool_false;
    }

    cut = end;
    while (cut > 0u && path->data[cut - 1u] != '/' && path->data[cut - 1u] != '\\') {
        --cut;
    }
    if (cut == 0u) {
        authum_path_clear(self);
        return authum_bool_false;
    }

    /* cut is index after the separator that ends the parent. */
    if (cut == 1u) {
        /* POSIX root `/…`. */
    } else if (cut == 3u && path->data[1] == ':' &&
               (path->data[2] == '/' || path->data[2] == '\\')) {
        /* Drive root `C:/…`. */
    } else {
        --cut; /* drop the trailing separator for normal parents */
        while (cut > 0u && (path->data[cut - 1u] == '/' || path->data[cut - 1u] == '\\')) {
            --cut;
        }
        if (cut == 0u) {
            cut = 1u;
        } else if (cut == 2u && path->data[1] == ':') {
            cut = 3u;
            if (path->size >= 3u && path->data[2] != '/' && path->data[2] != '\\') {
                cut = 2u;
            }
        }
    }

    if (cut >= (authum_usize_t)authum_path_capacity) {
        authum_path_clear(self);
        return authum_bool_false;
    }
    memcpy(self->data, path->data, cut);
    self->data[cut] = '\0';
    self->size = cut;
    return authum_bool_true;
}

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_PATH_H */

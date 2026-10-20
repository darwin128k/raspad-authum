/**
 * @file cfg.h
 * @brief Flat `key = value` config — get by key with defaults (no lh).
 *
 * Dialect:
 * - Trim whitespace on line / key / value.
 * - Empty lines and lines starting with `#` or `;` are skipped.
 * - First `=` splits key and value (value may contain more `=`).
 * - No `[sections]` required.
 *
 * Open once, then pull typed values. Missing or unparseable keys return the
 * caller-supplied default — that is the intended way to set defaults.
 *
 * @code
 * authum_cfg_t cfg;
 * if (authum_cfg_open(&cfg, &path)) {
 *     enable = authum_cfg_get_bool(&cfg, "EnableFoo", authum_bool_true);
 *     authum_cfg_get_cstr(&cfg, "Msg", buf, sizeof buf, "hello");
 *     authum_cfg_close(&cfg);
 * }
 * @endcode
 */

#ifndef AUTHUM_CFG_H
#define AUTHUM_CFG_H

#include <authum/bool.h>
#include <authum/byte.h>
#include <authum/char.h>
#include <authum/cstr.h>
#include <authum/null.h>
#include <authum/path.h>
#include <authum/size.h>

#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

enum
{
    authum_cfg_line_max = 512
};

/**
 * @struct authum_cfg
 * @brief Open flat-cfg file; getters rewind and scan for each key.
 */
struct authum_cfg
{
    FILE *file;
    authum_char_t line[authum_cfg_line_max];
};
typedef struct authum_cfg authum_cfg_t;

static inline void
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

static inline authum_bool_t
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

static inline authum_bool_t
authum_cfg_open(authum_cfg_t *self, const authum_path_t *path)
{
    return authum_cfg_open_cstr(self, authum_path_as_cstr_or_null(path));
}

static inline authum_bool_t
authum_cfg_adopt(authum_cfg_t *self, FILE *file)
{
    if (authum_null_eq(self) || authum_null_eq(file)) {
        return authum_bool_false;
    }
    self->file = file;
    self->line[0] = '\0';
    return authum_bool_true;
}

static inline authum_char_t *
authum_cfg_skip_spaces(authum_char_t *s)
{
    while (!authum_null_eq(s) && (*s == ' ' || *s == '\t')) {
        ++s;
    }
    return s;
}

static inline void
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

/**
 * @brief Look up @p key (case-insensitive). Value lives in @p self->line
 *        until the next getter call.
 * @return Pointer to value in @p self->line, or null if missing / not open.
 */
static inline const authum_char_t *
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
        /* Move value to the front of line so callers keep a stable pointer. */
        if (value != self->line) {
            authum_usize_t n = (authum_usize_t)strlen(value);
            memmove(self->line, value, n + 1u);
        }
        return self->line;
    }
    return authum_null;
}

/** @brief Get bool for @p key, or @p default_value if missing/unparseable. */
static inline authum_bool_t
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

/** @brief Get i32 for @p key, or @p default_value if missing/unparseable. */
static inline authum_i32_t
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

/** @brief Get u32 for @p key, or @p default_value if missing/unparseable. */
static inline authum_u32_t
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

/**
 * @brief Get string for @p key into @p buf, or @p default_value if missing.
 * @return ::authum_bool_true when a file value was copied (not the default).
 */
static inline authum_bool_t
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

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_CFG_H */

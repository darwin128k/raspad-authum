/**
 * @file cstr.h
 * @brief Small NUL-terminated string helpers for Authum modules (no lh).
 *
 * Prefer these over `strncpy` / `strcasecmp` so modules stay on Authum types
 * and avoid POSIX-only headers on Win32 HLDS builds.
 */

#ifndef AUTHUM_CSTR_H
#define AUTHUM_CSTR_H

#include <authum/bool.h>
#include <authum/byte.h>
#include <authum/char.h>
#include <authum/null.h>
#include <authum/size.h>

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline authum_char_t
authum_char_ascii_lower(authum_char_t c)
{
    if (c >= 'A' && c <= 'Z') {
        return (authum_char_t)(c - 'A' + 'a');
    }
    return c;
}

/**
 * @brief Copy @p src into @p dst (NUL-terminated). Truncates if needed.
 * @return ::authum_bool_false when @p dst is null/empty or @p src is null.
 */
static inline authum_bool_t
authum_cstr_copy(authum_char_t *dst, authum_usize_t dst_size, const authum_char_t *src)
{
    authum_usize_t n;

    if (authum_null_eq(dst) || dst_size == 0u || authum_null_eq(src)) {
        return authum_bool_false;
    }
    n = (authum_usize_t)strlen(src);
    if (n >= dst_size) {
        n = dst_size - 1u;
    }
    memcpy(dst, src, n);
    dst[n] = '\0';
    return authum_bool_true;
}

/**
 * @brief Equality of two C strings.
 * @param ignore_case ::authum_bool_true → ASCII case-fold; false → exact.
 */
static inline authum_bool_t
authum_cstr_eq(const authum_char_t *a, const authum_char_t *b, authum_bool_t ignore_case)
{
    if (authum_null_eq(a) || authum_null_eq(b)) {
        return (a == b) ? authum_bool_true : authum_bool_false;
    }
    if (!ignore_case) {
        return (strcmp(a, b) == 0) ? authum_bool_true : authum_bool_false;
    }
    for (;;) {
        authum_char_t ca = authum_char_ascii_lower(*a);
        authum_char_t cb = authum_char_ascii_lower(*b);
        if (ca != cb) {
            return authum_bool_false;
        }
        if (ca == '\0') {
            return authum_bool_true;
        }
        ++a;
        ++b;
    }
}

/**
 * @brief Parse a signed 32-bit decimal integer (optional leading `+`/`-`).
 */
static inline authum_bool_t
authum_cstr_parse_i32(const authum_char_t *text, authum_i32_t *out)
{
    const authum_char_t *p;
    authum_i32_t sign = 1;
    authum_u32_t acc = 0u;
    authum_bool_t any = authum_bool_false;

    if (authum_null_eq(text) || authum_null_eq(out)) {
        return authum_bool_false;
    }
    p = text;
    while (*p == ' ' || *p == '\t') {
        ++p;
    }
    if (*p == '+' || *p == '-') {
        if (*p == '-') {
            sign = -1;
        }
        ++p;
    }
    while (*p >= '0' && *p <= '9') {
        authum_u32_t digit = (authum_u32_t)(*p - '0');
        if (acc > (2147483647u - digit) / 10u) {
            return authum_bool_false;
        }
        acc = acc * 10u + digit;
        any = authum_bool_true;
        ++p;
    }
    while (*p == ' ' || *p == '\t') {
        ++p;
    }
    if (!any || *p != '\0') {
        return authum_bool_false;
    }
    if (sign < 0) {
        if (acc > 2147483648u) {
            return authum_bool_false;
        }
        if (acc == 2147483648u) {
            *out = (authum_i32_t)(-2147483647 - 1);
        } else {
            *out = -(authum_i32_t)acc;
        }
    } else {
        if (acc > 2147483647u) {
            return authum_bool_false;
        }
        *out = (authum_i32_t)acc;
    }
    return authum_bool_true;
}

/**
 * @brief Parse an unsigned 32-bit decimal integer.
 */
static inline authum_bool_t
authum_cstr_parse_u32(const authum_char_t *text, authum_u32_t *out)
{
    const authum_char_t *p;
    authum_u32_t acc = 0u;
    authum_bool_t any = authum_bool_false;

    if (authum_null_eq(text) || authum_null_eq(out)) {
        return authum_bool_false;
    }
    p = text;
    while (*p == ' ' || *p == '\t') {
        ++p;
    }
    if (*p == '+') {
        ++p;
    }
    while (*p >= '0' && *p <= '9') {
        authum_u32_t digit = (authum_u32_t)(*p - '0');
        if (acc > (0xffffffffu - digit) / 10u) {
            return authum_bool_false;
        }
        acc = acc * 10u + digit;
        any = authum_bool_true;
        ++p;
    }
    while (*p == ' ' || *p == '\t') {
        ++p;
    }
    if (!any || *p != '\0') {
        return authum_bool_false;
    }
    *out = acc;
    return authum_bool_true;
}

/**
 * @brief Parse a boolean: `0`/`1`, `true`/`false`, `yes`/`no`, `on`/`off`.
 *
 * Also accepts any other decimal integer (`atoi` style): nonzero → true.
 */
static inline authum_bool_t
authum_cstr_parse_bool(const authum_char_t *text, authum_bool_t *out)
{
    authum_i32_t n;

    if (authum_null_eq(text) || authum_null_eq(out)) {
        return authum_bool_false;
    }
    if (authum_cstr_eq(text, "true", authum_bool_true) ||
        authum_cstr_eq(text, "yes", authum_bool_true) ||
        authum_cstr_eq(text, "on", authum_bool_true)) {
        *out = authum_bool_true;
        return authum_bool_true;
    }
    if (authum_cstr_eq(text, "false", authum_bool_true) ||
        authum_cstr_eq(text, "no", authum_bool_true) ||
        authum_cstr_eq(text, "off", authum_bool_true)) {
        *out = authum_bool_false;
        return authum_bool_true;
    }
    if (!authum_cstr_parse_i32(text, &n)) {
        return authum_bool_false;
    }
    *out = (n != 0) ? authum_bool_true : authum_bool_false;
    return authum_bool_true;
}

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_CSTR_H */

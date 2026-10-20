#include <authum/cstr.h>

authum_char_t
authum_char_ascii_lower(authum_char_t c)
{
    if (c >= 'A' && c <= 'Z') {
        return (authum_char_t)(c - 'A' + 'a');
    }
    return c;
}

authum_bool_t
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

authum_bool_t
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

authum_bool_t
authum_cstr_parse_i32(const authum_char_t *text, authum_i32_t *out)
{
    const authum_char_t *p;
    authum_i32_t sign = 1;
    authum_u32_t acc = 0u;
    authum_u32_t limit;
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
    limit = (sign < 0) ? 2147483648u : 2147483647u;
    while (*p >= '0' && *p <= '9') {
        authum_u32_t digit = (authum_u32_t)(*p - '0');
        if (acc > (limit - digit) / 10u) {
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

authum_bool_t
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

authum_bool_t
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

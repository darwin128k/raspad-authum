#include <authum/cstr.h>

#include <lh/numeric/types.h>
#include <lh/str/parse/ullong.h>

#include <string.h>

authum_char_t
authum_char_ascii_lower(authum_char_t c)
{
    if (c >= 'A' && c <= 'Z')
    {
        return (authum_char_t)(c - 'A' + 'a');
    }
    return c;
}

authum_bool_t
authum_cstr_copy(authum_char_t *dst, authum_usize_t dst_size, const authum_char_t *src)
{
    authum_usize_t n;

    if (authum_null_eq(dst) || dst_size == 0u || authum_null_eq(src))
    {
        return authum_bool_false;
    }
    n = (authum_usize_t)strlen(src);
    if (n >= dst_size)
    {
        n = dst_size - 1u;
    }
    memcpy(dst, src, n);
    dst[n] = '\0';
    return authum_bool_true;
}

authum_bool_t
authum_cstr_eq(const authum_char_t *a, const authum_char_t *b, authum_bool_t ignore_case)
{
    if (authum_null_eq(a) || authum_null_eq(b))
    {
        return (a == b) ? authum_bool_true : authum_bool_false;
    }
    if (!ignore_case)
    {
        return (strcmp(a, b) == 0) ? authum_bool_true : authum_bool_false;
    }
    for (;;)
    {
        authum_char_t ca = authum_char_ascii_lower(*a);
        authum_char_t cb = authum_char_ascii_lower(*b);
        if (ca != cb)
        {
            return authum_bool_false;
        }
        if (ca == '\0')
        {
            return authum_bool_true;
        }
        ++a;
        ++b;
    }
}

/*
 * The digits of a number written as text: spaces and tabs around it dropped,
 * an optional `+` (or `-` when @p allow_minus) taken off. False when nothing
 * is left; whether the rest really is digits is the parser's call.
 */
authum_bool_t
authum_cstr_number_digits(const authum_char_t *text, authum_bool_t allow_minus,
                          authum_bool_t *negative, const authum_char_t **digits,
                          authum_usize_t *size)
{
    const authum_char_t *end;

    while (*text == ' ' || *text == '	')
    {
        ++text;
    }
    *negative = authum_bool_false;
    if (*text == '+' || (allow_minus && *text == '-'))
    {
        *negative = (*text == '-') ? authum_bool_true : authum_bool_false;
        ++text;
    }
    end = text + strlen(text);
    while (end > text && (end[-1] == ' ' || end[-1] == '	'))
    {
        --end;
    }
    *digits = text;
    *size = (authum_usize_t)(end - text);
    return *size > 0u ? authum_bool_true : authum_bool_false;
}

authum_bool_t
authum_cstr_parse_i32(const authum_char_t *text, authum_i32_t *out)
{
    const authum_char_t *digits;
    authum_usize_t size;
    authum_bool_t negative;
    lh_ullong_t magnitude;

    if (authum_null_eq(text) || authum_null_eq(out) ||
        !authum_cstr_number_digits(text, authum_bool_true, &negative, &digits, &size) ||
        !lh_str_ptr_parse_ullong_digits(digits, (lh_usize_t)size,
                                        negative ? 2147483648ULL : 2147483647ULL, &magnitude))
    {
        return authum_bool_false;
    }
    *out = negative ? (authum_i32_t)(-(lh_sllong_t)magnitude) : (authum_i32_t)magnitude;
    return authum_bool_true;
}

authum_bool_t
authum_cstr_parse_u32(const authum_char_t *text, authum_u32_t *out)
{
    const authum_char_t *digits;
    authum_usize_t size;
    authum_bool_t negative;
    lh_ullong_t value;

    if (authum_null_eq(text) || authum_null_eq(out) ||
        !authum_cstr_number_digits(text, authum_bool_false, &negative, &digits, &size) ||
        !lh_str_ptr_parse_ullong_digits(digits, (lh_usize_t)size, 0xffffffffULL, &value))
    {
        return authum_bool_false;
    }
    *out = (authum_u32_t)value;
    return authum_bool_true;
}

authum_bool_t
authum_cstr_parse_bool(const authum_char_t *text, authum_bool_t *out)
{
    authum_i32_t n;

    if (authum_null_eq(text) || authum_null_eq(out))
    {
        return authum_bool_false;
    }
    if (authum_cstr_eq(text, "true", authum_bool_true) ||
        authum_cstr_eq(text, "yes", authum_bool_true) ||
        authum_cstr_eq(text, "on", authum_bool_true))
    {
        *out = authum_bool_true;
        return authum_bool_true;
    }
    if (authum_cstr_eq(text, "false", authum_bool_true) ||
        authum_cstr_eq(text, "no", authum_bool_true) ||
        authum_cstr_eq(text, "off", authum_bool_true))
    {
        *out = authum_bool_false;
        return authum_bool_true;
    }
    if (!authum_cstr_parse_i32(text, &n))
    {
        return authum_bool_false;
    }
    *out = (n != 0) ? authum_bool_true : authum_bool_false;
    return authum_bool_true;
}

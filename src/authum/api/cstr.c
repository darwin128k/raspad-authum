#include <authum/cstr.h>
#include <lh/char/letter.h>
#include <lh/util/char.h>
#include <lh/memory/std.h>

#include <lh/str/parse/ullong.h>
#include <lh/util/str/ptr.h>

authum_char_t
authum_char_to_lower(authum_char_t c)
{
    return lh_char_is_upper(c) ? lh_char_fold_case(c) : c;
}

authum_bool_t
authum_cstr_copy(authum_char_t *dst, authum_usize_t dst_size, const authum_char_t *src)
{
    authum_usize_t n;

    if (authum_null_eq(dst) || lh_math_eq(dst_size, 0u) || authum_null_eq(src))
    {
        return authum_bool_false;
    }
    n = lh_cast_static(authum_usize_t, lh_str_ptr_len(src));
    if (lh_math_ge(n, dst_size))
    {
        n = lh_math_sub(dst_size, 1u);
    }
    lh_memory_std_copy(dst, src, n);
    dst[n] = lh_char_map_nul;
    return authum_bool_true;
}

authum_bool_t
authum_cstr_is_equal(const authum_char_t *a, const authum_char_t *b, authum_bool_t ignore_case)
{
    if (authum_null_eq(a) || authum_null_eq(b))
    {
        return (lh_math_eq(a, b)) ? authum_bool_true : authum_bool_false;
    }
    if (!ignore_case)
    {
        return lh_str_ptr_equals(a, b, lh_bool_false) ? authum_bool_true : authum_bool_false;
    }
    for (;;)
    {
        authum_char_t ca = authum_char_to_lower(lh_ptr_deref(a));
        authum_char_t cb = authum_char_to_lower(lh_ptr_deref(b));
        if (lh_math_ne(ca, cb))
        {
            return authum_bool_false;
        }
        if (lh_math_eq(ca, lh_char_map_nul))
        {
            return authum_bool_true;
        }
        a = lh_math_add_one(a);
        b = lh_math_add_one(b);
    }
}

authum_bool_t
authum_cstr_contains(const authum_char_t *haystack, const authum_char_t *needle)
{
    if (authum_null_eq(haystack) || authum_null_eq(needle))
    {
        return authum_bool_false;
    }
    return lh_str_ptr_contains(haystack, needle, lh_bool_false) ? authum_bool_true
                                                                : authum_bool_false;
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

    while (lh_math_eq(lh_ptr_deref(text), lh_char_map_space) ||
           lh_math_eq(lh_ptr_deref(text), lh_char_map_ht))
    {
        text = lh_math_add_one(text);
    }
    lh_ptr_deref(negative) = authum_bool_false;
    if (lh_math_eq(lh_ptr_deref(text), lh_char_map_plus) ||
        (allow_minus && lh_math_eq(lh_ptr_deref(text), lh_char_map_minus)))
    {
        lh_ptr_deref(negative) = (lh_math_eq(lh_ptr_deref(text), lh_char_map_minus))
                                     ? authum_bool_true
                                     : authum_bool_false;
        text = lh_math_add_one(text);
    }
    end = lh_math_add(text, lh_str_ptr_len(text));
    while (lh_math_gt(end, text) &&
           (lh_math_eq(end[-1], lh_char_map_space) || lh_math_eq(end[-1], lh_char_map_ht)))
    {
        end = lh_math_sub_one(end);
    }
    lh_ptr_deref(digits) = text;
    lh_ptr_deref(size) = (authum_usize_t)(lh_math_sub(end, text));
    return lh_math_gt(lh_ptr_deref(size), 0u) ? authum_bool_true : authum_bool_false;
}

authum_bool_t
authum_cstr_parse_i32(const authum_char_t *text, authum_i32_t *out)
{
    const authum_char_t *digits;
    authum_usize_t size;
    authum_bool_t negative;
    lh_ullong_t magnitude;

    if (authum_null_eq(text) || authum_null_eq(out) ||
        !authum_cstr_number_digits(text, authum_bool_true, lh_addr_of(negative), lh_addr_of(digits),
                                   lh_addr_of(size)) ||
        !lh_str_ptr_parse_ullong_digits(digits, lh_cast_static(lh_usize_t, size),
                                        negative ? 2147483648ULL : 2147483647ULL,
                                        lh_addr_of(magnitude)))
    {
        return authum_bool_false;
    }
    lh_ptr_deref(out) = negative ? (authum_i32_t)(-lh_cast_static(lh_sllong_t, magnitude))
                                 : lh_cast_static(authum_i32_t, magnitude);
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
        !authum_cstr_number_digits(text, authum_bool_false, lh_addr_of(negative),
                                   lh_addr_of(digits), lh_addr_of(size)) ||
        !lh_str_ptr_parse_ullong_digits(digits, lh_cast_static(lh_usize_t, size), 0xffffffffULL,
                                        lh_addr_of(value)))
    {
        return authum_bool_false;
    }
    lh_ptr_deref(out) = lh_cast_static(authum_u32_t, value);
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
    if (authum_cstr_is_equal(text, "true", authum_bool_true) ||
        authum_cstr_is_equal(text, "yes", authum_bool_true) ||
        authum_cstr_is_equal(text, "on", authum_bool_true))
    {
        lh_ptr_deref(out) = authum_bool_true;
        return authum_bool_true;
    }
    if (authum_cstr_is_equal(text, "false", authum_bool_true) ||
        authum_cstr_is_equal(text, "no", authum_bool_true) ||
        authum_cstr_is_equal(text, "off", authum_bool_true))
    {
        lh_ptr_deref(out) = authum_bool_false;
        return authum_bool_true;
    }
    if (!authum_cstr_parse_i32(text, lh_addr_of(n)))
    {
        return authum_bool_false;
    }
    lh_ptr_deref(out) = (lh_math_ne(n, 0)) ? authum_bool_true : authum_bool_false;
    return authum_bool_true;
}

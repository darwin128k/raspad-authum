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
#include <authum/char.h>
#include <authum/extern.h>
#include <authum/null.h>
#include <authum/size.h>

#include <stdarg.h>

AUTHUM_EXTERN_C_BEGIN

authum_char_t
authum_char_to_lower(authum_char_t c);

/**
 * @brief Copy @p src into @p dst (NUL-terminated). Truncates if needed.
 * @return ::authum_bool_false when @p dst is null/empty or @p src is null.
 */
authum_bool_t
authum_cstr_copy(authum_char_t *dst, authum_usize_t dst_size, const authum_char_t *src);

/**
 * @brief Format @p fmt and the arguments into @p dst (NUL-terminated).
 *
 * The conversions of the Authum string formatter: `%%`, `%c`, `%s`, `%d`, `%i`,
 * `%u`, `%x`, `%X`; the flags `0` (zero-pad) and `-` (left-justify); the field
 * width; a `.` precision for `%s`. There is no `%f`, no `%p` and no `+` sign.
 *
 * A result that does not fit is cut: @p dst holds the beginning of it and the
 * rest is dropped.
 *
 * @param dst      Destination buffer.
 * @param dst_size Capacity of @p dst in characters, the NUL included.
 * @param fmt      Format mask.
 * @param ...      One argument per conversion in @p fmt.
 * @return Characters written into @p dst, without the NUL; 0 when @p dst is
 *         null/empty, @p fmt is null or the mask holds an unknown conversion.
 */
authum_usize_t
authum_cstr_format(authum_char_t *dst, authum_usize_t dst_size, const authum_char_t *fmt, ...);

/**
 * @brief ::authum_cstr_format with the arguments in @p ap.
 *
 * @param dst      Destination buffer.
 * @param dst_size Capacity of @p dst in characters, the NUL included.
 * @param fmt      Format mask.
 * @param ap       Arguments, one per conversion in @p fmt.
 * @return Characters written into @p dst, without the NUL; 0 when @p dst is
 *         null/empty, @p fmt is null or the mask holds an unknown conversion.
 */
authum_usize_t
authum_cstr_format_v(authum_char_t *dst, authum_usize_t dst_size, const authum_char_t *fmt,
                     va_list ap);

/**
 * @brief Equality of two C strings.
 * @param ignore_case ::authum_bool_true → ASCII case-fold; false → exact.
 */
authum_bool_t
authum_cstr_is_equal(const authum_char_t *a, const authum_char_t *b, authum_bool_t ignore_case);

/**
 * @brief True when @p haystack holds @p needle (exact bytes, case-sensitive).
 * @return ::authum_bool_false when either pointer is null.
 */
authum_bool_t
authum_cstr_contains(const authum_char_t *haystack, const authum_char_t *needle);

/**
 * @brief Parse a signed 32-bit decimal integer (optional leading `+`/`-`).
 */
authum_bool_t
authum_cstr_parse_i32(const authum_char_t *text, authum_i32_t *out);

/**
 * @brief Parse an unsigned 32-bit decimal integer.
 */
authum_bool_t
authum_cstr_parse_u32(const authum_char_t *text, authum_u32_t *out);

/**
 * @brief Parse a boolean: `0`/`1`, `true`/`false`, `yes`/`no`, `on`/`off`.
 *
 * Also accepts any other decimal integer (`atoi` style): nonzero → true.
 */
authum_bool_t
authum_cstr_parse_bool(const authum_char_t *text, authum_bool_t *out);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_CSTR_H */

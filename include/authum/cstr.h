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
#include <authum/float.h>
#include <authum/null.h>
#include <authum/size.h>

#include <stdarg.h>

AUTHUM_EXTERN_C_BEGIN

authum_char_t
authum_char_to_lower(authum_char_t c);

/**
 * @brief True when @p c is a decimal digit (`0`–`9`).
 */
authum_bool_t
authum_char_is_digit(authum_char_t c);

/**
 * @brief True when @p c is a letter (`a`–`z`, `A`–`Z`).
 */
authum_bool_t
authum_char_is_letter(authum_char_t c);

/**
 * @brief True when @p c is a lower-case letter.
 */
authum_bool_t
authum_char_is_lower(authum_char_t c);

/**
 * @brief True when @p c is an upper-case letter.
 */
authum_bool_t
authum_char_is_upper(authum_char_t c);

/**
 * @brief True when @p c is a space, a tab or any other white space.
 */
authum_bool_t
authum_char_is_space(authum_char_t c);

/**
 * @brief True when @p c is a hexadecimal digit (`0`–`9`, `a`–`f`, `A`–`F`).
 */
authum_bool_t
authum_char_is_xdigit(authum_char_t c);

/**
 * @brief @p c as an upper-case letter; other characters come back unchanged.
 */
authum_char_t
authum_char_to_upper(authum_char_t c);

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
 * @brief Parse a decimal number into a single-precision float.
 *
 * The whole text must be the number: `-? digits? (. digits?)? ([eE] [+-]? digits)?`.
 * A value too big for ::authum_float_t is refused, as is a text with
 * anything else in it.
 *
 * @param text Number text.
 * @param out  Receives the value; untouched when the text is not a number.
 * @return ::authum_bool_true when @p text is a number.
 */
authum_bool_t
authum_cstr_parse_float(const authum_char_t *text, authum_float_t *out);

/**
 * @brief Parse a boolean: `0`/`1`, `true`/`false`, `yes`/`no`, `on`/`off`.
 *
 * Also accepts any other decimal integer (`atoi` style): nonzero → true.
 */
authum_bool_t
authum_cstr_parse_bool(const authum_char_t *text, authum_bool_t *out);

/**
 * @brief Length of @p text in characters, the NUL not counted.
 * @return 0 when @p text is null.
 */
authum_usize_t
authum_cstr_len(const authum_char_t *text);

/**
 * @brief Equality of the first @p size characters of @p a and @p b.
 *
 * A string shorter than @p size compares unequal to a longer one.
 *
 * @param ignore_case ::authum_bool_true → ASCII case-fold; false → exact.
 */
authum_bool_t
authum_cstr_is_equal_n(const authum_char_t *a, const authum_char_t *b, authum_usize_t size,
                       authum_bool_t ignore_case);

/**
 * @brief True when @p text begins with @p prefix.
 *
 * @param ignore_case ::authum_bool_true → ASCII case-fold; false → exact.
 */
authum_bool_t
authum_cstr_starts_with(const authum_char_t *text, const authum_char_t *prefix,
                        authum_bool_t ignore_case);

/**
 * @brief True when @p text ends with @p suffix.
 *
 * @param ignore_case ::authum_bool_true → ASCII case-fold; false → exact.
 */
authum_bool_t
authum_cstr_ends_with(const authum_char_t *text, const authum_char_t *suffix,
                      authum_bool_t ignore_case);

/**
 * @brief The place where @p needle first appears in @p haystack.
 * @return Null when there is none, or when a pointer is null.
 */
const authum_char_t *
authum_cstr_find(const authum_char_t *haystack, const authum_char_t *needle);

/**
 * @brief The place of the first @p c in @p text.
 * @return Null when there is none, or when @p text is null.
 */
const authum_char_t *
authum_cstr_find_char(const authum_char_t *text, authum_char_t c);

/**
 * @brief Write @p size bytes as hexadecimal text — two characters per byte,
 *        high nibble first — into @p dst.
 *
 * A buffer too small for the whole text takes the part that fits; the cut is
 * at a byte boundary. What was written is NUL-terminated when @p dst has room
 * for the terminator.
 *
 * @param bytes     Bytes to write; may be null when @p size is 0.
 * @param size      Number of bytes to write.
 * @param dst       Destination buffer.
 * @param dst_size  Capacity of @p dst in characters, the NUL included.
 * @param uppercase ::authum_bool_true for `A`–`F`, false for `a`–`f`.
 *
 * @return Characters written, the NUL not counted; 0 when there is nothing to
 *         write or @p dst has no room.
 */
authum_usize_t
authum_cstr_format_hex_bytes(const authum_byte_t *bytes, authum_usize_t size, authum_char_t *dst,
                             authum_usize_t dst_size, authum_bool_t uppercase);

/**
 * @brief Parse hexadecimal digits from the start of @p text into @p out — two
 *        digits make one byte.
 *
 * Exactly @p out_size bytes are read; what follows them is left alone. Upper
 * and lower case digits are both accepted. @p out is written only when the
 * whole text is good: a text that is too short or holds a character that is
 * not a hexadecimal digit leaves @p out untouched.
 *
 * @param text     Text to parse.
 * @param out      Receives the parsed bytes on success; untouched on failure.
 * @param out_size Number of bytes to parse.
 *
 * @return ::authum_bool_true when @p out_size bytes were parsed.
 */
authum_bool_t
authum_cstr_parse_hex_bytes(const authum_char_t *text, authum_byte_t *out, authum_usize_t out_size);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_CSTR_H */

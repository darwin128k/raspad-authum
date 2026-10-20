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
extern "C"
{
#endif

    authum_char_t
    authum_char_ascii_lower(authum_char_t c);

    /**
     * @brief Copy @p src into @p dst (NUL-terminated). Truncates if needed.
     * @return ::authum_bool_false when @p dst is null/empty or @p src is null.
     */
    authum_bool_t
    authum_cstr_copy(authum_char_t *dst, authum_usize_t dst_size, const authum_char_t *src);

    /**
     * @brief Equality of two C strings.
     * @param ignore_case ::authum_bool_true → ASCII case-fold; false → exact.
     */
    authum_bool_t
    authum_cstr_eq(const authum_char_t *a, const authum_char_t *b, authum_bool_t ignore_case);

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

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_CSTR_H */

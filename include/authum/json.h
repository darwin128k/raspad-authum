/**
 * @file json.h
 * @brief Small JSON object field getters (flat objects / one level).
 *
 * Enough for API replies like IPHub (`{"countryCode":"RU","asn":15169,…}`).
 * The text is read whole by a strict RFC 8259 reader (lh/json): a key is only
 * a member of the top-level object, never text inside a string, and every
 * escape (`\n`, `é`, surrogate pairs) is decoded. Text that is not JSON,
 * or not an object, has no fields.
 */

#ifndef AUTHUM_JSON_H
#define AUTHUM_JSON_H

#include <authum/bool.h>
#include <authum/char.h>
#include <authum/extern.h>
#include <authum/null.h>
#include <authum/size.h>

AUTHUM_EXTERN_C_BEGIN

/**
 * @brief Copy a string field @p key from a JSON object into @p out.
 * @return false when missing / not a string / bad args.
 */
authum_bool_t
authum_json_object_get_cstr(const authum_char_t *json, const authum_char_t *key,
                            authum_char_t *out, authum_usize_t out_size);

/**
 * @brief Read an integer field @p key (optional leading `-`).
 * @return false when missing / not a number.
 */
authum_bool_t
authum_json_object_get_i32(const authum_char_t *json, const authum_char_t *key,
                           authum_i32_t *out);

/**
 * @brief Like ::authum_json_object_get_i32, or @p fallback when missing.
 */
authum_i32_t
authum_json_object_get_i32_or(const authum_char_t *json, const authum_char_t *key,
                              authum_i32_t fallback);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_JSON_H */

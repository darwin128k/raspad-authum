/**
 * @file json.h
 * @brief Small JSON object field getters (flat objects / one level).
 *
 * Enough for API replies like IPHub (`{"countryCode":"RU","asn":15169,…}`).
 * Not a full DOM — no arrays-as-trees, no streaming. Escapes in strings: `\\`
 * and `\"` only.
 */

#ifndef AUTHUM_JSON_H
#define AUTHUM_JSON_H

#include <authum/bool.h>
#include <authum/byte.h>
#include <authum/char.h>
#include <authum/null.h>
#include <authum/size.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Copy a string field @p key from a JSON object into @p out.
 * @return false when missing / not a string / bad args.
 */
authum_bool_t authum_json_object_get_cstr(const authum_char_t *json, const authum_char_t *key,
					  authum_char_t *out, authum_usize_t out_size);

/**
 * @brief Read an integer field @p key (optional leading `-`).
 * @return false when missing / not a number.
 */
authum_bool_t authum_json_object_get_i32(const authum_char_t *json, const authum_char_t *key,
					 authum_i32_t *out);

/**
 * @brief Like ::authum_json_object_get_i32, or @p fallback when missing.
 */
authum_i32_t authum_json_object_get_i32_or(const authum_char_t *json, const authum_char_t *key,
					   authum_i32_t fallback);

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_JSON_H */

/**
 * @file private.h
 * @brief Private to `json.c`: the functions below its public API.
 *
 * Not installed: modules never see these.
 */

#ifndef AUTHUM_API_JSON_PRIVATE_H
#define AUTHUM_API_JSON_PRIVATE_H

#include <authum/json.h>
#include <lh/core/char/map.h>
#include <lh/core/memory/std.h>
#include <lh/core/web/json.h>

AUTHUM_EXTERN_C_BEGIN

/**
 * @brief Read @p json into @p root; the caller deinits @p root whatever this returns.
 */
authum_bool_t
authum_json_read(const authum_char_t *json, lh_web_json_node_t *root);

/**
 * @brief The member @p key of the object @p root, or null.
 */
const lh_web_json_node_t *
authum_json_object_find(const lh_web_json_node_t *root, const authum_char_t *key);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_API_JSON_PRIVATE_H */

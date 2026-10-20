/**
 * @file item.h
 * @brief Declarative key → field bindings for ::authum_cfg_t.
 *
 * Defaults live on the module’s config object (static init). Apply only
 * overwrites a field when the key is present and parses.
 */

#ifndef AUTHUM_CFG_ITEM_H
#define AUTHUM_CFG_ITEM_H

#include <authum/bool.h>
#include <authum/byte.h>
#include <authum/cfg/kind.h>
#include <authum/char.h>
#include <authum/cstr.h>
#include <authum/null.h>
#include <authum/ptr.h>
#include <authum/size.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct authum_cfg_item
 * @brief One `key` mapped to a field at @p ptr.
 *
 * For ::authum_cfg_kind_cstr, @p size is the destination buffer capacity.
 * For scalar kinds, @p size is ignored.
 */
struct authum_cfg_item
{
    const authum_char_t *key;
    authum_cfg_kind_t kind;
    authum_ptr ptr;
    authum_usize_t size;
};
typedef struct authum_cfg_item authum_cfg_item_t;

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_CFG_ITEM_H */

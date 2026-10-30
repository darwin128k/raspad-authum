/**
 * @file cfg.h
 * @brief Flat `key = value` config — get / apply by key (no lh).
 *
 * Implementation lives in Authum’s `authum_client` static library
 * (`src/authum/api/cfg.c`). Modules `#include <authum/cfg.h>` and link
 * `authum_client` — they do not compile Authum sources themselves.
 *
 * Dialect:
 * - Trim whitespace on line / key / value.
 * - Empty lines and lines starting with `#` or `;` are skipped.
 * - First `=` splits key and value (value may contain more `=`).
 * - No `[sections]` required.
 *
 * Prefer: defaults on your config object (static init), then
 * ::authum_cfg_apply_items. ::authum_cfg_get_* remain for one-shot reads with
 * an explicit default.
 *
 * @code
 * mymod_config_t g = { .enable_foo = authum_bool_true, ... };
 * authum_cfg_t cfg;
 * static const authum_cfg_item_t items[] = {
 *     { "enable_foo", authum_cfg_kind_bool, &g.enable_foo, 0 },
 * };
 * if (authum_cfg_open(&cfg, &path)) {
 *     authum_cfg_apply_items(&cfg, items, sizeof items / sizeof items[0]);
 *     authum_cfg_close(&cfg);
 * }
 * @endcode
 */

#ifndef AUTHUM_CFG_H
#define AUTHUM_CFG_H

#include <authum/cfg/item.h>
#include <authum/path.h>

enum
{
    authum_cfg_line_max = 512
};

/**
 * @struct authum_cfg
 * @brief Open flat-cfg file; getters scan it for each key.
 *
 * The file is read whole when it is opened; every getter scans the text
 * from its first line, the way the old stream was rewound and read.
 */
struct authum_cfg
{
    /** The config text, owned by the cfg; null when nothing is open. */
    authum_char_t *text;
    /** Bytes in @p text, without the terminator. */
    authum_usize_t size;
    authum_char_t line[authum_cfg_line_max];
};
typedef struct authum_cfg authum_cfg_t;

AUTHUM_EXTERN_C_BEGIN

void
authum_cfg_close(authum_cfg_t *self);

authum_bool_t
authum_cfg_open_cstr(authum_cfg_t *self, const authum_char_t *path);

authum_bool_t
authum_cfg_open(authum_cfg_t *self, const authum_path_t *path);

/**
 * @brief Open a config that is already in memory.
 *
 * The text is copied; the cfg owns its copy. A missing file and an
 * unreadable one both leave the cfg closed.
 *
 * @return ::authum_bool_true when the config is open.
 */
authum_bool_t
authum_cfg_open_text(authum_cfg_t *self, const authum_char_t *text, authum_usize_t size);

/**
 * @brief Look up @p key (case-insensitive). Value lives in @p self->line
 *        until the next getter / apply call.
 */
const authum_char_t *
authum_cfg_get_value(authum_cfg_t *self, const authum_char_t *key);

authum_bool_t
authum_cfg_get_bool(authum_cfg_t *self, const authum_char_t *key, authum_bool_t default_value);

authum_i32_t
authum_cfg_get_i32(authum_cfg_t *self, const authum_char_t *key, authum_i32_t default_value);

authum_u32_t
authum_cfg_get_u32(authum_cfg_t *self, const authum_char_t *key, authum_u32_t default_value);

/**
 * @brief Copy the value of @p key into @p buf, or @p default_value when absent.
 * @return ::authum_bool_true when the key was present (false → default copied).
 */
authum_bool_t
authum_cfg_copy_cstr(authum_cfg_t *self, const authum_char_t *key, authum_char_t *buf,
                     authum_usize_t buf_size, const authum_char_t *default_value);

authum_bool_t
authum_cfg_apply_bool(authum_cfg_t *self, const authum_char_t *key, authum_bool_t *out);

authum_bool_t
authum_cfg_apply_i32(authum_cfg_t *self, const authum_char_t *key, authum_i32_t *out);

authum_bool_t
authum_cfg_apply_u32(authum_cfg_t *self, const authum_char_t *key, authum_u32_t *out);

authum_bool_t
authum_cfg_apply_cstr(authum_cfg_t *self, const authum_char_t *key, authum_char_t *buf,
                      authum_usize_t buf_size);

authum_bool_t
authum_cfg_apply_item(authum_cfg_t *self, const authum_cfg_item_t *item);

void
authum_cfg_apply_items(authum_cfg_t *self, const authum_cfg_item_t *items,
                       authum_usize_t count);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_CFG_H */

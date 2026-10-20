/**
 * @file path.h
 * @brief Module-owned filesystem path (::authum_path_t) — no lh.
 *
 * Fixed-capacity, NUL-terminated text. Prefer ready-made directories from
 * ::authum_env_t; use ::authum_path_join only to append a leaf
 * (`qproto.cfg`, `sprites/foo.spr`, …).
 *
 * Host-filled env paths use the platform-native form (via lh inside
 * `authum_mm`). Module joins always insert `/` (::authum_path_sep); `/`
 * still works with `fopen` on Windows HLDS.
 */

#ifndef AUTHUM_PATH_H
#define AUTHUM_PATH_H

#include <authum/bool.h>
#include <authum/char.h>
#include <authum/cstr.h>
#include <authum/null.h>
#include <authum/path/capacity.h>
#include <authum/path/sep.h>
#include <authum/size.h>

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct authum_path
 * @brief Absolute or relative path text of at most ::authum_path_capacity - 1.
 */
struct authum_path
{
    authum_char_t data[authum_path_capacity];
    /** Payload length without the trailing NUL. */
    authum_usize_t size;
};
typedef struct authum_path authum_path_t;

void
authum_path_clear(authum_path_t *self);

authum_bool_t
authum_path_is_empty(const authum_path_t *self);

/**
 * @brief C string view of @p self. Never null — empty path yields `""`.
 */
const authum_char_t *
authum_path_as_cstr(const authum_path_t *self);

/**
 * @brief Like ::authum_path_as_cstr, but null when @p self is null/empty.
 */
const authum_char_t *
authum_path_as_cstr_or_null(const authum_path_t *self);

authum_bool_t
authum_path_set_cstr(authum_path_t *self, const authum_char_t *text);

authum_bool_t
authum_path_assign(authum_path_t *self, const authum_path_t *other);

authum_bool_t
authum_path_ends_with_sep(const authum_path_t *self);

/**
 * @brief Write @p dir + `/` + @p leaf into @p self (NUL-terminated).
 *
 * @p leaf may contain further `/` segments (`sprites/foo.spr`). Skips a
 * duplicate separator when @p dir already ends with `/` or `\\`.
 */
authum_bool_t
authum_path_join(authum_path_t *self, const authum_path_t *dir, const authum_char_t *leaf);

/**
 * @brief Drop the last segment of @p path into @p self.
 *
 * Treats both `/` and `\\` as separators. Examples: `a/b/c` → `a/b`,
 * `/a` → `/`, `C:/a` → `C:/`. Fails when there is no parent segment.
 */
authum_bool_t
authum_path_parent(authum_path_t *self, const authum_path_t *path);

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_PATH_H */

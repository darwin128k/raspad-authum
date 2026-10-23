/**
 * @file loader.h
 * @brief Authum's policy on top of ::lh_os_loader_t.
 *
 * The mechanism (open, method table, start, unload children) is lh.
 * Authum decides which paths: `configs/modules.list` beside the image when
 * present (each entry is `modules/<name>.so`), otherwise shared libraries in
 * `modules/` beside the image; skip
 * `authum_mm`. One rejected child does not fail the list. A missing
 * directory succeeds with nothing loaded.
 */

#ifndef AUTHUM_LOADER_H
#define AUTHUM_LOADER_H

#include <authum/module.h>
#include <lh/bool.h>
#include <lh/compiler/extern/c.h>
#include <lh/fs/path.h>
#include <lh/index.h>
#include <lh/size.h>

/**
 * @def AUTHUM_MODULES_DIR
 * @brief Directory name beside a module image that holds extra modules.
 */
#define AUTHUM_MODULES_DIR "modules"

/**
 * @def AUTHUM_MODULES_LIST
 * @brief Optional load list, `configs/modules.list` beside a module image
 *        (Authum list dialect).
 *
 * One path per line; optional `[linux]` / `[win32]` / `[modules]` / `[any]`
 * sections. Not INI — parsed with ::lh_str_line_reader_t in Authum.
 */
#define AUTHUM_MODULES_LIST "modules.list"

LH_COMPILER_EXTERN_C_BEGIN

/**
 * @brief Bind this image, hand it ::authum_module_ops, and start it.
 *
 * @return ::lh_bool_true if the image was bound and ::authum_module_start
 *         accepted it.
 */
lh_bool_t
authum_loader_init(void);

/**
 * @brief Stop ::authum (children first) and release it.
 */
void
authum_loader_deinit(void);

/**
 * @brief Load @p path as a child of @p self.
 *
 * @p self must already hold the loader privilege. The host plugin file
 * (`authum_mm`) is rejected without opening. On false, the reason is in
 * ::lh_os_last_error or ::lh_os_system_last_error.
 *
 * @return ::lh_bool_true if the child was kept.
 */
lh_bool_t
authum_loader_load(authum_module_t *self, const lh_fs_path_t *path);

/**
 * @brief Load every shared image in @p dir (one level).
 *
 * A missing @p dir or a path that is not a directory succeeds with nothing
 * loaded. Empty @p dir is an error.
 *
 * @return ::lh_bool_true if the directory was scanned or did not exist.
 */
lh_bool_t
authum_loader_load_dir(authum_module_t *self, const lh_fs_path_t *dir);

/**
 * @brief Load module paths listed in @p path (::AUTHUM_MODULES_LIST dialect).
 *
 * Lines before a section and under `[modules]` / `[any]` apply on every OS.
 * `[linux]` / `[win32]` only on that OS. Each non-comment line is one path.
 * Relative paths join under `modules/` beside @p self's image; absolute
 * paths are used as-is. The host image is skipped. One rejected load does
 * not fail the list. A parse error returns false
 * (::authum_error_code_bad_modules_list).
 *
 * @return ::lh_bool_true if the file was read and parsed.
 */
lh_bool_t
authum_loader_load_list(authum_module_t *self, const lh_fs_path_t *path);

/**
 * @brief Load children for @p self: `configs/modules.list` beside the image, else
 *        shared images from `modules/`.
 *
 * When `modules.list` is present, only that list is used (an empty matching
 * section loads nothing). When it is missing, the directory scan runs.
 * The image file itself is skipped.
 */
lh_bool_t
authum_loader_load_modules(authum_module_t *self);

/**
 * @brief Unload every child of @p self. @p self stays loaded.
 */
void
authum_loader_unload_all(authum_module_t *self);

/**
 * @brief Number of children currently stored (not counting @p self).
 */
lh_usize_t
authum_loader_get_loaded(const authum_module_t *self);

/**
 * @brief First child in load order, or ::lh_null if there is none.
 */
authum_module_t *
authum_loader_get_first(const authum_module_t *self);

/**
 * @brief Child loaded after @p child, or ::lh_null if @p child is the last.
 */
authum_module_t *
authum_loader_get_next(const authum_module_t *self, authum_module_t *child);

/**
 * @brief Child number @p index in load order, or ::lh_null if there are not that many.
 *
 * Walk every child with ::authum_loader_get_first and ::authum_loader_get_next.
 */
authum_module_t *
authum_loader_get(authum_module_t *self, lh_uindex_t index);

/**
 * @brief Child whose `authum_module_info.name` equals @p name (case-insensitive).
 */
authum_module_t *
authum_loader_find(authum_module_t *self, const char *name);

/**
 * @brief Resolve @p name under `modules/` (package folder or bare `.so`) and load it.
 *
 * Fails if a child with that info name is already loaded, or the image cannot
 * be resolved / started. On false, see ::lh_os_last_error.
 */
lh_bool_t
authum_loader_load_name(authum_module_t *self, const char *name);

/**
 * @brief Stop and unload the child named @p name. False if not loaded.
 */
lh_bool_t
authum_loader_unload_name(authum_module_t *self, const char *name);

/**
 * @brief Unload then load again from the same image path (or `modules/<name>`).
 */
lh_bool_t
authum_loader_reload_name(authum_module_t *self, const char *name);

LH_COMPILER_EXTERN_C_END

#endif /* AUTHUM_LOADER_H */

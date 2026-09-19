/**
 * @file loader.h
 * @brief Loader capability on ::authum_module_t (PHP trait / LoaderInterface).
 *
 * Any module may load children into its table. Scan policy (`modules/`,
 * skip `authum_mm`) is Authum; the image itself is ::lh_os_shared_module_t.
 */

#ifndef AUTHUM_LOADER_H
#define AUTHUM_LOADER_H

#include <authum/module.h>
#include <lh/bool.h>
#include <lh/compiler/extern/c.h>
#include <lh/size.h>
#include <lh/str/ptr.h>

/**
 * @def AUTHUM_MODULES_DIR
 * @brief Directory name beside a module image that holds extra modules.
 */
#define AUTHUM_MODULES_DIR "modules"

LH_COMPILER_EXTERN_C_BEGIN

/**
 * @brief Initialize this image's ::authum (bind ::authum_module_info).
 */
void
authum_loader_init(void);

/**
 * @brief Unload children of ::authum and release its table.
 */
void
authum_loader_deinit(void);

/**
 * @brief Open @p path and append a child to @p self.
 *
 * On false, ::lh_os_get_last_error. The path is still the caller's — log both.
 *
 * @param self Module that owns the child table.
 * @param path Filesystem path for ::authum_module_load.
 * @return ::lh_bool_true if the library opened, identity was accepted, and stored.
 */
lh_bool_t
authum_loader_load(authum_module_t *self, lh_str_cptr path);

/**
 * @brief Load every Authum module from @p dir (one level, not recursive).
 *
 * Skips directories, shortcuts, and the host plugin (`authum_mm`). Only
 * shared libraries are opened; a library without ::AUTHUM_MODULE_INFO_SYM
 * is skipped. A missing @p dir succeeds with nothing loaded.
 *
 * @param self Module that owns the child table.
 * @param dir  Directory to list. ::lh_null or empty is an error.
 * @return ::lh_bool_true if the directory was scanned or did not exist.
 */
lh_bool_t
authum_loader_load_dir(authum_module_t *self, lh_str_cptr dir);

/**
 * @brief Load modules from `modules` next to @p plugin_path.
 *
 * @param self        Module that owns the child table.
 * @param plugin_path Path of that module's image (`.dll` / `.so`).
 * @return ::lh_bool_true if the sibling directory was scanned or did not exist.
 */
lh_bool_t
authum_loader_load_modules(authum_module_t *self, lh_str_cptr plugin_path);

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
 * @brief Child at @p index, or ::lh_null if out of range.
 */
authum_module_t *
authum_loader_get(authum_module_t *self, lh_uindex_t index);

LH_COMPILER_EXTERN_C_END

#endif /* AUTHUM_LOADER_H */

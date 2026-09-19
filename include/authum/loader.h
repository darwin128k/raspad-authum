/**
 * @file loader.h
 * @brief Loader of ::authum_module_t — table is an ::lh_vector_t.
 */

#ifndef AUTHUM_LOADER_H
#define AUTHUM_LOADER_H

#include <authum/loader/fields.h>
#include <authum/module.h>
#include <lh/bool.h>
#include <lh/compiler/extern/c.h>
#include <lh/size.h>
#include <lh/str/ptr.h>
#include <lh/vector.h>

/**
 * @def AUTHUM_MODULES_DIR
 * @brief Directory name beside the host plugin that holds extra modules.
 */
#define AUTHUM_MODULES_DIR "modules"

/**
 * @struct authum_loader
 * @brief Owns a growable list of loaded modules.
 *
 * Fields are injected via ::authum_loader_fields.
 */
struct authum_loader
{
    authum_loader_fields(lh_vector_t);
};

/**
 * @typedef authum_loader_t
 * @brief Alias for `struct authum_loader`.
 */
typedef struct authum_loader authum_loader_t;

LH_COMPILER_EXTERN_C_BEGIN

/**
 * @brief Process-wide loader used by the Metamod plugin.
 */
extern authum_loader_t authum_loader;

/**
 * @brief Initialize the process-wide ::authum_loader (empty vector).
 */
void
authum_loader_init(void);

/**
 * @brief Unload every module and release the vector allocation.
 */
void
authum_loader_deinit(void);

/**
 * @brief Open @p path and append the module to @p self.
 *
 * On false, ::lh_os_get_last_error. The path is still the caller's — log both.
 *
 * @param self Loader that owns the module table.
 * @param path Filesystem path for ::authum_module_load.
 * @return ::lh_bool_true if the library opened, identity was accepted, and stored.
 */
lh_bool_t
authum_loader_load(authum_loader_t *self, lh_str_cptr path);

/**
 * @brief Load every Authum module from @p dir (one level, not recursive).
 *
 * Skips directories, shortcuts, and the host plugin (`authum_mm`). Only
 * shared libraries (`.dll` / `.so`) are opened; a library without
 * ::AUTHUM_MODULE_INFO_SYM is skipped. A missing @p dir succeeds with
 * nothing loaded.
 *
 * @param self Loader that owns the module table.
 * @param dir  Directory to list. ::lh_null or empty is an error.
 * @return ::lh_bool_true if the directory was scanned or did not exist.
 */
lh_bool_t
authum_loader_load_dir(authum_loader_t *self, lh_str_cptr dir);

/**
 * @brief Load modules from `modules` next to the host plugin DLL.
 *
 * ::lh_os_fs_path_dir of @p plugin_path, then ::lh_os_fs_path_join
 * `modules`, then ::authum_loader_load_dir.
 *
 * @param self        Loader that owns the module table.
 * @param plugin_path Path from Metamod `GET_PLUGIN_PATH` (the `.dll` / `.so`).
 * @return ::lh_bool_true if the sibling directory was scanned or did not exist.
 */
lh_bool_t
authum_loader_load_modules(authum_loader_t *self, lh_str_cptr plugin_path);

/**
 * @brief Unload every stored module and clear the vector (capacity kept).
 * @param self Loader whose table is emptied.
 */
void
authum_loader_unload_all(authum_loader_t *self);

/**
 * @brief Number of currently stored (loaded) modules.
 * @param self Loader to inspect.
 */
lh_usize_t
authum_loader_get_loaded(const authum_loader_t *self);

/**
 * @brief Module at @p index, or ::lh_null if out of range.
 *
 * @param self  Loader to inspect.
 * @param index Zero-based index into the loaded table.
 */
authum_module_t *
authum_loader_get(authum_loader_t *self, lh_uindex_t index);

LH_COMPILER_EXTERN_C_END

#endif /* AUTHUM_LOADER_H */

/**
 * @file module.h
 * @brief One loaded shared library.
 *
 * Open/close go through ::lh_os_shared_open / ::lh_os_shared_close.
 * After open, the module must export ::AUTHUM_MODULE_INFO_SYM; ABI is checked
 * with ::lh_version_is_compatible against ::AUTHUM_MODULE_ABI.
 * Failure reason: ::lh_os_get_last_error.
 */

#ifndef AUTHUM_MODULE_H
#define AUTHUM_MODULE_H

#include <authum/module/info.h>
#include <authum/module/fields.h>
#include <lh/bool.h>
#include <lh/compiler/extern/c.h>
#include <lh/os/shared.h>
#include <lh/str/ptr.h>

/**
 * @struct authum_module
 * @brief One DLL/.so currently owned by the loader.
 *
 * Fields are injected via ::authum_module_fields.
 */
struct authum_module
{
    authum_module_fields(lh_os_shared_handle_t, const authum_module_info_t *);
};

/**
 * @typedef authum_module_t
 * @brief Alias for `struct authum_module`.
 */
typedef struct authum_module authum_module_t;

LH_COMPILER_EXTERN_C_BEGIN

/**
 * @brief Mark @p self unloaded (handle ::lh_null). Does not close a live handle.
 */
void
authum_module_init(authum_module_t *self);

/**
 * @brief True if @p self currently holds an OS handle.
 */
lh_bool_t
authum_module_is_loaded(const authum_module_t *self);

/**
 * @brief Open @p path into an unloaded slot.
 *
 * Already-loaded slots fail with last-error “already loaded”.
 *
 * @param self Unloaded module slot.
 * @param path Filesystem path for ::lh_os_shared_open.
 * @return ::lh_bool_true if the library opened and identity was accepted.
 *         On false, ::lh_os_get_last_error.
 */
lh_bool_t
authum_module_load(authum_module_t *self, lh_str_cptr path);

/**
 * @brief Close a loaded slot. Unloaded slots succeed without work.
 *
 * @param self Module slot to close.
 * @return ::lh_bool_false if the OS close failed (::lh_os_get_last_error).
 */
lh_bool_t
authum_module_unload(authum_module_t *self);

/**
 * @brief Exported symbol @p name, or ::lh_null if unloaded / missing.
 *
 * Unloaded slots set last-error “not loaded”.
 *
 * @param self Module slot to search.
 * @param name Exported symbol name (narrow).
 */
lh_ptr
authum_module_get_sym(const authum_module_t *self, lh_str_cptr name);

/**
 * @brief True if loaded and @p name is exported.
 */
lh_bool_t
authum_module_has_sym(const authum_module_t *self, lh_str_cptr name);

/**
 * @brief Identity from ::AUTHUM_MODULE_INFO_SYM, or ::lh_null if unloaded.
 */
const authum_module_info_t *
authum_module_get_info(const authum_module_t *self);

LH_COMPILER_EXTERN_C_END

#endif /* AUTHUM_MODULE_H */

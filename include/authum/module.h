/**
 * @file module.h
 * @brief One Authum module — a shared image plus ::authum_module_info.
 *
 * The OS image is ::lh_os_shared_module_t (first fields). After open, the
 * library must export ::AUTHUM_MODULE_INFO_SYM; ABI is checked with
 * ::lh_version_is_compatible against ::AUTHUM_MODULE_ABI.
 * Failure reason: ::lh_os_get_last_error.
 *
 * Loading children is ::authum_loader_* on this same type.
 */

#ifndef AUTHUM_MODULE_H
#define AUTHUM_MODULE_H

#include <authum/module/info.h>
#include <authum/module/fields.h>
#include <lh/bool.h>
#include <lh/compiler/extern/c.h>
#include <lh/os/shared.h>
#include <lh/os/shared/module.h>
#include <lh/str/ptr.h>
#include <lh/vector.h>

/**
 * @struct authum_module
 * @brief Shared image with Authum identity.
 *
 * Fields are injected via ::authum_module_fields.
 */
struct authum_module
{
    authum_module_fields(lh_os_shared_handle_t, lh_bool_t, const authum_module_info_t *,
                         lh_vector_t);
};

/**
 * @typedef authum_module_t
 * @brief Alias for `struct authum_module`.
 */
typedef struct authum_module authum_module_t;

LH_COMPILER_EXTERN_C_BEGIN

/**
 * @brief This image's module (handle, info, children).
 */
extern authum_module_t authum;

/**
 * @brief Unloaded slot: no handle, empty child table.
 */
void
authum_module_init(authum_module_t *self);

/**
 * @brief Unload children and this handle, then release the child table.
 */
void
authum_module_deinit(authum_module_t *self);

/**
 * @brief True if @p self currently holds an OS handle.
 */
lh_bool_t
authum_module_is_loaded(const authum_module_t *self);

/**
 * @brief Bind an already-loaded image that contains @p info (the host).
 *
 * @param self Unloaded module slot.
 * @param info Exported ::authum_module_info in this image.
 * @return ::lh_bool_true if the image was found and ABI accepted.
 */
lh_bool_t
authum_module_bind(authum_module_t *self, const authum_module_info_t *info);

/**
 * @brief Open @p path into an unloaded slot and read ::AUTHUM_MODULE_INFO_SYM.
 *
 * Already-loaded slots fail with last-error “already loaded”.
 *
 * @param self Unloaded module slot.
 * @param path Filesystem path for ::lh_os_shared_module_open.
 * @return ::lh_bool_true if the library opened and identity was accepted.
 *         On false, ::lh_os_get_last_error.
 */
lh_bool_t
authum_module_load(authum_module_t *self, lh_str_cptr path);

/**
 * @brief Unload every child, then close this handle.
 *
 * @param self Module slot to close.
 * @return ::lh_bool_false if the OS close failed (::lh_os_get_last_error).
 */
lh_bool_t
authum_module_unload(authum_module_t *self);

/**
 * @brief Exported symbol @p name, or ::lh_null if unloaded / missing.
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

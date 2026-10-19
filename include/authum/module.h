/**
 * @file module.h
 * @brief An Authum module is an ::lh_os_module_t.
 *
 * Identity (::authum_module_info_t) is read from ::AUTHUM_MODULE_INFO_SYM
 * and kept in the module's data. Loading children is the loader privilege
 * (::lh_os_module_grant_loader), granted from ::authum_module_start. A
 * child image exports ::lh_os_module_ops_t under ::AUTHUM_MODULE_OPS_SYM.
 * Its start is this module's business: the lh loader only calls it.
 */

#ifndef AUTHUM_MODULE_H
#define AUTHUM_MODULE_H

#include <authum/module/info.h>
#include <lh/bool.h>
#include <lh/compiler/extern/c.h>
#include <lh/os/module.h>

#ifndef AUTHUM_MODULE_EXPORT
#    if defined(_WIN32) || defined(__CYGWIN__)
#        define AUTHUM_MODULE_EXPORT __declspec(dllexport)
#    elif defined(__GNUC__)
#        define AUTHUM_MODULE_EXPORT __attribute__((visibility("default")))
#    else
#        define AUTHUM_MODULE_EXPORT
#    endif
#endif

/**
 * @typedef authum_module_t
 * @brief The lh module. Authum does not add a second object around it.
 */
typedef lh_os_module_t authum_module_t;

/**
 * @def AUTHUM_MODULE_OPS_SYM
 * @brief Exported name of the child's ::lh_os_module_ops_t.
 */
#define AUTHUM_MODULE_OPS_SYM "authum_module_ops"

LH_COMPILER_EXTERN_C_BEGIN

/**
 * @brief This image's module.
 */
extern authum_module_t authum;

/**
 * @brief Method table this image exports as ::AUTHUM_MODULE_OPS_SYM.
 *
 * ::authum_module_start accepts ::AUTHUM_MODULE_INFO_SYM, grants the
 * loader, and — when this module was loaded by another — loads `modules/`
 * beside its image. The host has no owner, so that scan is
 * ::authum_loader_load_modules.
 */
AUTHUM_MODULE_EXPORT extern const lh_os_module_ops_t authum_module_ops;

/**
 * @brief Bring @p self up: identity, loader privilege, nested `modules/`.
 *
 * A child image's ::lh_os_module_ops_t may point its `start` here.
 *
 * @return ::lh_bool_false when identity is missing or the ABI does not match.
 */
AUTHUM_MODULE_EXPORT lh_bool_t
authum_module_start(authum_module_t *self);

/**
 * @brief Drop the identity pointer. Children are already unloaded.
 *
 * A child image's ::lh_os_module_ops_t may point its `stop` here.
 */
AUTHUM_MODULE_EXPORT void
authum_module_stop(authum_module_t *self);

/**
 * @brief Identity stored by ::authum_module_start, or ::lh_null.
 */
const authum_module_info_t *
authum_module_get_info(const authum_module_t *self);

LH_COMPILER_EXTERN_C_END

#endif /* AUTHUM_MODULE_H */

/**
 * @file module.h
 * @brief Host-side module object (lh-backed). Client modules use ::authum/client.h.
 */

#ifndef AUTHUM_MODULE_H
#define AUTHUM_MODULE_H

#include <authum/export.h>
#include <authum/module/info.h>
#include <lh/bool.h>
#include <lh/compiler/extern/c.h>
#include <lh/os/module.h>

/**
 * @typedef authum_module_t
 * @brief The lh module object used inside `authum_mm`.
 */
typedef lh_os_module_t authum_module_t;

/**
 * @def AUTHUM_MODULE_OPS_SYM
 * @brief Host / legacy ops table name (not used by lh-free client modules).
 */
#define AUTHUM_MODULE_OPS_SYM "authum_module_ops"

LH_COMPILER_EXTERN_C_BEGIN

extern authum_module_t authum;

AUTHUM_MODULE_EXPORT extern const lh_os_module_ops_t authum_module_ops;

AUTHUM_MODULE_EXPORT lh_bool_t
authum_module_start(authum_module_t *self);

AUTHUM_MODULE_EXPORT void
authum_module_stop(authum_module_t *self);

const authum_module_info_t *
authum_module_get_info(const authum_module_t *self);

LH_COMPILER_EXTERN_C_END

#endif /* AUTHUM_MODULE_H */

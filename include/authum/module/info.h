/**
 * @file info.h
 * @brief Module identity exported as `authum_module_info` (no lh dependency).
 */

#ifndef AUTHUM_MODULE_INFO_H
#define AUTHUM_MODULE_INFO_H

#include <authum/char.h>
#include <authum/export.h>
#include <authum/extern.h>
#include <authum/module/info/fields.h>
#include <authum/version.h>

AUTHUM_EXTERN_C_BEGIN

/**
 * @def AUTHUM_MODULE_INFO_SYM
 * @brief Exported symbol name of ::authum_module_info_t.
 */
#define AUTHUM_MODULE_INFO_SYM "authum_module_info"

/**
 * @def AUTHUM_MODULE_ABI
 * @brief Loader ABI (`1.0.0`) — lh-free module API (`authum_module_api`).
 *
 * 1.0: every host table starts with `size` and only grows at the end
 * (<authum/abi.h>), so a module loads on any 1.x host; the engine facade
 * covers players, cvars and client queries. A minor bump adds functions, a
 * major bump removes or changes one. Checked with
 * ::authum_version_is_abi_compatible.
 */
#define AUTHUM_MODULE_ABI AUTHUM_VERSION(1, 0, 0)

struct authum_module_info
{
    authum_module_info_fields(const authum_char_t *, authum_version_t);
};

typedef struct authum_module_info authum_module_info_t;

AUTHUM_MODULE_EXPORT extern authum_module_info_t authum_module_info;

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_MODULE_INFO_H */

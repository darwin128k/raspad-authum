/**
 * @file info.h
 * @brief Loaded-module identity (::authum_module_info_t).
 *
 * Each Authum module — including this host image — exports one
 * ::authum_module_info_t named ::AUTHUM_MODULE_INFO_SYM (`authum_module_info`).
 * ::authum_module_start reads it after the image is open and keeps the
 * pointer on the module. This is not Metamod's `plugin_info_t`; that table
 * is filled from this one.
 *
 * `abi` is the loader contract the module was built against (::AUTHUM_MODULE_ABI).
 * `version` is the module's own SemVer.
 */

#ifndef AUTHUM_MODULE_INFO_H
#define AUTHUM_MODULE_INFO_H

#include <authum/module/info/fields.h>
#include <lh/compiler/extern/c.h>
#include <lh/datetime.h>
#include <lh/str/ptr.h>
#include <lh/version.h>
#include <lh/version/initializer.h>

/**
 * @def AUTHUM_MODULE_INFO_SYM
 * @brief Exported symbol name of ::authum_module_info_t (unmangled C).
 */
#define AUTHUM_MODULE_INFO_SYM "authum_module_info"

/**
 * @def AUTHUM_MODULE_ABI
 * @brief Loader ABI this host speaks (`0.1.0`).
 *
 * A module is accepted when ::lh_version_is_compatible(module.abi, this).
 */
#define AUTHUM_MODULE_ABI lh_version_initializer(0, 1, 0)

/**
 * @struct authum_module_info
 * @brief Identity a loaded module reports about itself.
 *
 * Fields are injected via ::authum_module_info_fields.
 */
struct authum_module_info
{
    authum_module_info_fields(lh_str_cptr, lh_version_t, lh_datetime_t);
};

/**
 * @typedef authum_module_info_t
 * @brief Alias for `struct authum_module_info`.
 */
typedef struct authum_module_info authum_module_info_t;

LH_COMPILER_EXTERN_C_BEGIN

/**
 * @brief Identity this image exports as ::AUTHUM_MODULE_INFO_SYM.
 */
extern authum_module_info_t authum_module_info;

LH_COMPILER_EXTERN_C_END

#endif /* AUTHUM_MODULE_INFO_H */

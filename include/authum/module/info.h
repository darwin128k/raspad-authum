/**
 * @file info.h
 * @brief Module identity exported as `authum_module_info` (no lh dependency).
 */

#ifndef AUTHUM_MODULE_INFO_H
#define AUTHUM_MODULE_INFO_H

#include <authum/char.h>
#include <authum/export.h>
#include <authum/module/info/fields.h>
#include <authum/version.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @def AUTHUM_MODULE_INFO_SYM
 * @brief Exported symbol name of ::authum_module_info_t.
 */
#define AUTHUM_MODULE_INFO_SYM "authum_module_info"

/**
 * @def AUTHUM_MODULE_ABI
 * @brief Loader ABI (`0.3.0`) — lh-free module API (`authum_module_api`);
 *        0.3 adds the per-module sandbox and event bus to ::authum_host_t.
 */
#define AUTHUM_MODULE_ABI AUTHUM_VERSION(0, 3, 0)

    struct authum_module_info
    {
        authum_module_info_fields(const authum_char_t *, authum_version_t);
    };

    typedef struct authum_module_info authum_module_info_t;

    AUTHUM_MODULE_EXPORT extern authum_module_info_t authum_module_info;

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_MODULE_INFO_H */

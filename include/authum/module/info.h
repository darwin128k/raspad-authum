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

/**
 * @def AUTHUM_MODULE_INFO_SYM
 * @brief Exported symbol name of ::authum_module_info_t.
 */
#define AUTHUM_MODULE_INFO_SYM "authum_module_info"

/**
 * @def AUTHUM_MODULE_ABI
 * @brief Loader ABI (`2.0.0`) — lh-free module API (`authum_module_api`).
 *
 * 2.0 (Authum 0.4.0) starts over: no 1.x module loads, and the host tables
 * carry no size to check. From here the ABI is frozen: nothing that exists
 * changes or moves. New API is added at the end of a table, or as a new
 * table or a new structure, and bumps the minor; an input structure a module
 * fills in never grows (a new field is a new structure). A module therefore
 * runs on a host of the same major and the same or a later minor, which
 * ::authum_version_is_abi_compatible checks at load.
 */
#define AUTHUM_MODULE_ABI AUTHUM_VERSION(2, 0, 0)

struct authum_module_info
{
    authum_module_info_fields(const authum_char_t *, authum_version_t);
};

typedef struct authum_module_info authum_module_info_t;

AUTHUM_EXTERN_C_BEGIN

AUTHUM_MODULE_EXPORT extern authum_module_info_t authum_module_info;

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_MODULE_INFO_H */

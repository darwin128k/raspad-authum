/**
 * @file api.h
 * @brief Child module lifecycle table (`authum_module_api`) — no lh dependency.
 *
 * The host looks this symbol up, fills ::authum_host_t, and calls `start`.
 * Do **not** export `authum_module_ops` (lh) from a client module.
 */

#ifndef AUTHUM_MODULE_API_H
#define AUTHUM_MODULE_API_H

#include <authum/bool.h>
#include <authum/export.h>
#include <authum/extern.h>
#include <authum/host.h>

AUTHUM_EXTERN_C_BEGIN

#define AUTHUM_MODULE_API_SYM "authum_module_api"

/**
 * @struct authum_module_api
 * @brief Module entry points. `start` receives a host facade (copy what you need).
 */
typedef struct authum_module_api
{
    /** @return ::authum_bool_true to stay loaded. */
    authum_bool_t (*start)(authum_host_t *host);
    void (*stop)(void);
} authum_module_api_t;

AUTHUM_MODULE_EXPORT extern const authum_module_api_t authum_module_api;

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_MODULE_API_H */

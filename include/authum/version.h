/**
 * @file version.h
 * @brief SemVer triple for module ABI / package version (no lh dependency).
 */

#ifndef AUTHUM_VERSION_H
#define AUTHUM_VERSION_H

#include <authum/bool.h>
#include <authum/extern.h>
#include <authum/null.h>

typedef struct authum_version
{
    authum_u32_t major;
    authum_u32_t minor;
    authum_u32_t patch;
} authum_version_t;

/** Brace-init helper: `authum_version_t v = AUTHUM_VERSION(1, 2, 3);` */
#define AUTHUM_VERSION(ma, mi, pa) {(authum_u32_t)(ma), (authum_u32_t)(mi), (authum_u32_t)(pa)}

AUTHUM_EXTERN_C_BEGIN

/**
 * @brief Loader ABI gate: same major; for 0.y.z also same minor and patch >= need.
 */
authum_bool_t
authum_version_is_compatible(const authum_version_t *have, const authum_version_t *need);

/**
 * @brief Loader gate for ::AUTHUM_MODULE_ABI: may a module built for
 *        @p module run on a host built for @p host?
 *
 * The major must match. The host's minor must be the module's or later: a
 * later minor only appends API, and a module built against it may call what
 * an older host does not have. The patch does not matter.
 */
authum_bool_t
authum_version_is_abi_compatible(const authum_version_t *module, const authum_version_t *host);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_VERSION_H */

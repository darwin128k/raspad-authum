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
 * Same major is enough from 1.0 on: the size-checked tables
 * (<authum/abi.h>) cover a minor difference either way. 0.y.z needs the
 * same minor.
 */
authum_bool_t
authum_version_is_abi_compatible(const authum_version_t *module, const authum_version_t *host);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_VERSION_H */

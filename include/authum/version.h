/**
 * @file version.h
 * @brief SemVer triple for module ABI / package version (no lh dependency).
 */

#ifndef AUTHUM_VERSION_H
#define AUTHUM_VERSION_H

#include <authum/bool.h>
#include <authum/byte.h>
#include <authum/null.h>

typedef struct authum_version
{
    authum_u32_t major;
    authum_u32_t minor;
    authum_u32_t patch;
} authum_version_t;

/** Brace-init helper: `authum_version_t v = AUTHUM_VERSION(1, 2, 3);` */
#define AUTHUM_VERSION(ma, mi, pa)                                                                 \
    {                                                                                              \
        (authum_u32_t)(ma), (authum_u32_t)(mi), (authum_u32_t)(pa)                                 \
    }

/**
 * @brief Loader ABI gate: same major; for 0.y.z also same minor and patch >= need.
 */
static inline authum_bool_t
authum_version_is_compatible(const authum_version_t *have, const authum_version_t *need)
{
    if (authum_null_eq(have) || authum_null_eq(need)) {
        return authum_bool_false;
    }
    if (have->major != need->major) {
        return authum_bool_false;
    }
    if (need->major == 0u) {
        return (have->minor == need->minor && have->patch >= need->patch) ? authum_bool_true
                                                                         : authum_bool_false;
    }
    if (have->minor != need->minor) {
        return (have->minor > need->minor) ? authum_bool_true : authum_bool_false;
    }
    return (have->patch >= need->patch) ? authum_bool_true : authum_bool_false;
}

#endif /* AUTHUM_VERSION_H */

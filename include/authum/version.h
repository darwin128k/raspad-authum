/**
 * @file version.h
 * @brief SemVer triple for module ABI / package version (no lh dependency).
 */

#ifndef AUTHUM_VERSION_H
#define AUTHUM_VERSION_H

#include <authum/bool.h>

typedef struct authum_version
{
    unsigned major;
    unsigned minor;
    unsigned patch;
} authum_version_t;

/** Brace-init helper: `authum_version_t v = AUTHUM_VERSION(1, 2, 3);` */
#define AUTHUM_VERSION(ma, mi, pa)                                                                 \
    {                                                                                              \
        (unsigned)(ma), (unsigned)(mi), (unsigned)(pa)                                             \
    }

/**
 * @brief Loader ABI gate: same major; for 0.y.z also same minor and patch >= need.
 */
static inline authum_bool_t
authum_version_is_compatible(const authum_version_t *have, const authum_version_t *need)
{
    if (have == 0 || need == 0) {
        return AUTHUM_FALSE;
    }
    if (have->major != need->major) {
        return AUTHUM_FALSE;
    }
    if (need->major == 0u) {
        return (have->minor == need->minor && have->patch >= need->patch) ? AUTHUM_TRUE
                                                                         : AUTHUM_FALSE;
    }
    if (have->minor != need->minor) {
        return (have->minor > need->minor) ? AUTHUM_TRUE : AUTHUM_FALSE;
    }
    return (have->patch >= need->patch) ? AUTHUM_TRUE : AUTHUM_FALSE;
}

#endif /* AUTHUM_VERSION_H */

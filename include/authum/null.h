/**
 * @file null.h
 * @brief Null pointer constant for Authum modules (no lh).
 *
 * Mirrors ::lh_null.
 */

#ifndef AUTHUM_NULL_H
#define AUTHUM_NULL_H

#include <authum/ptr.h>

/**
 * @def authum_null
 * @brief Null pointer (::authum_ptr).
 */
#define authum_null ((authum_ptr)0)

/**
 * @def authum_null_eq(p)
 * @brief 1 when @p p is null.
 */
#define authum_null_eq(p) ((p) == authum_null)

#endif /* AUTHUM_NULL_H */

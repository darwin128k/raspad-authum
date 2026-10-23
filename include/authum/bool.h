/**
 * @file bool.h
 * @brief Boolean type for Authum modules (no lh).
 *
 * Mirrors ::lh_bool_t / ::lh_bool_false / ::lh_bool_true.
 */

#ifndef AUTHUM_BOOL_H
#define AUTHUM_BOOL_H

#include <authum/byte.h>

/**
 * @def authum_bool_false
 * @brief Boolean false (0).
 */
#define authum_bool_false 0

/**
 * @def authum_bool_true
 * @brief Boolean true (1).
 */
#define authum_bool_true 1

/**
 * @typedef authum_bool_t
 * @brief Minimal boolean (::authum_byte_t); nonzero is true.
 */
typedef authum_byte_t authum_bool_t;

#endif /* AUTHUM_BOOL_H */

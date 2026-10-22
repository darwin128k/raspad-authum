/**
 * @file float.h
 * @brief Single-precision floating point for Authum modules (no lh).
 *
 * Mirrors ::lh_float_t and its limits. `float` is also what HLDS uses for
 * times and vectors (`gpGlobals->time`, `vec3_t`), so payloads carry it as is.
 */

#ifndef AUTHUM_FLOAT_H
#define AUTHUM_FLOAT_H

#include <float.h>

/**
 * @typedef authum_float_t
 * @brief Single-precision floating point (`float`).
 */
typedef float authum_float_t;

/**
 * @def AUTHUM_FLOAT_T_MIN
 * @brief Smallest positive normalized ::authum_float_t (`FLT_MIN`).
 */
#define AUTHUM_FLOAT_T_MIN FLT_MIN

/**
 * @def AUTHUM_FLOAT_T_MAX
 * @brief Maximum finite ::authum_float_t (`FLT_MAX`).
 */
#define AUTHUM_FLOAT_T_MAX FLT_MAX

/**
 * @def AUTHUM_FLOAT_T_EPS
 * @brief Machine epsilon of ::authum_float_t (`FLT_EPSILON`).
 */
#define AUTHUM_FLOAT_T_EPS FLT_EPSILON

#endif /* AUTHUM_FLOAT_H */

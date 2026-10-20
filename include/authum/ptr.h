/**
 * @file ptr.h
 * @brief Untyped pointer for Authum modules (no lh).
 *
 * Mirrors ::lh_ptr. Prefer typed pointers in APIs; use this for opaque spans.
 */

#ifndef AUTHUM_PTR_H
#define AUTHUM_PTR_H

/**
 * @typedef authum_ptr
 * @brief `void *` under an Authum name.
 */
typedef void *authum_ptr;

#endif /* AUTHUM_PTR_H */

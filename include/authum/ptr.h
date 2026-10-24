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

/**
 * @typedef authum_cptr
 * @brief `const void *` under an Authum name — for data a call only reads.
 *
 * Not the same as `const authum_ptr`, which is `void *const` (the pointer
 * is constant, the bytes are not).
 */
typedef const void *authum_cptr;

#endif /* AUTHUM_PTR_H */

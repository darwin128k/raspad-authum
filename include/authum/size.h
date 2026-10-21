/**
 * @file size.h
 * @brief Unsigned size type for Authum modules (no lh).
 *
 * Mirrors ::lh_usize_t for the module ABI. Authum modules are 32-bit HLDS, so
 * this is a 32-bit unsigned integer — not C `size_t` from a foreign libc.
 */

#ifndef AUTHUM_SIZE_H
#define AUTHUM_SIZE_H

#include <authum/byte.h>

/**
 * @typedef authum_usize_t
 * @brief Unsigned size / length in bytes (or element counts).
 */
typedef authum_u32_t authum_usize_t;

#endif /* AUTHUM_SIZE_H */

/**
 * @file kind.h
 * @brief SHA algorithm selector for ::authum_crypto_sha_t.
 *
 * Mirrors ::lh_crypto_sha_kind_* (subset exposed to modules; no lh).
 */

#ifndef AUTHUM_CRYPTO_SHA_KIND_H
#define AUTHUM_CRYPTO_SHA_KIND_H

#include <authum/byte.h>

/**
 * @def authum_crypto_sha_kind_1
 * @brief SHA-1 (160-bit digest).
 */
#define authum_crypto_sha_kind_1 0

/**
 * @def authum_crypto_sha_kind_224
 * @brief SHA-224.
 */
#define authum_crypto_sha_kind_224 1

/**
 * @def authum_crypto_sha_kind_256
 * @brief SHA-256.
 */
#define authum_crypto_sha_kind_256 2

/**
 * @def authum_crypto_sha_kind_384
 * @brief SHA-384.
 */
#define authum_crypto_sha_kind_384 3

/**
 * @def authum_crypto_sha_kind_512
 * @brief SHA-512.
 */
#define authum_crypto_sha_kind_512 4

/**
 * @typedef authum_crypto_sha_kind_t
 * @brief One of the `authum_crypto_sha_kind_*` constants.
 */
typedef authum_byte_t authum_crypto_sha_kind_t;

#endif /* AUTHUM_CRYPTO_SHA_KIND_H */

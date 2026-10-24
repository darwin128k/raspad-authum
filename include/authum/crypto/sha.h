/**
 * @file sha.h
 * @brief SHA hasher type for Authum modules (lh under the hood; no lh).
 *
 * Same idea as ::lh_crypto_sha_t: stack session + init / update / finish /
 * hash via <authum/crypto.h>. Compression internals stay in lh.
 */

#ifndef AUTHUM_CRYPTO_SHA_H
#define AUTHUM_CRYPTO_SHA_H

#include <authum/crypto/sha/kind.h>
#include <authum/extern.h>
#include <authum/size.h>

AUTHUM_EXTERN_C_BEGIN

/**
 * @def AUTHUM_CRYPTO_SHA1_DIGEST_SIZE
 * @brief SHA-1 digest length in bytes (20).
 */
#define AUTHUM_CRYPTO_SHA1_DIGEST_SIZE 20u

/**
 * @def AUTHUM_CRYPTO_SHA224_DIGEST_SIZE
 * @brief SHA-224 digest length in bytes (28).
 */
#define AUTHUM_CRYPTO_SHA224_DIGEST_SIZE 28u

/**
 * @def AUTHUM_CRYPTO_SHA256_DIGEST_SIZE
 * @brief SHA-256 digest length in bytes (32).
 */
#define AUTHUM_CRYPTO_SHA256_DIGEST_SIZE 32u

/**
 * @def AUTHUM_CRYPTO_SHA384_DIGEST_SIZE
 * @brief SHA-384 digest length in bytes (48).
 */
#define AUTHUM_CRYPTO_SHA384_DIGEST_SIZE 48u

/**
 * @def AUTHUM_CRYPTO_SHA512_DIGEST_SIZE
 * @brief SHA-512 digest length in bytes (64).
 */
#define AUTHUM_CRYPTO_SHA512_DIGEST_SIZE 64u

/**
 * @def AUTHUM_CRYPTO_SHA_DIGEST_SIZE_MAX
 * @brief Longest digest this hasher can produce.
 */
#define AUTHUM_CRYPTO_SHA_DIGEST_SIZE_MAX AUTHUM_CRYPTO_SHA512_DIGEST_SIZE

/**
 * @def AUTHUM_CRYPTO_SHA_STORAGE
 * @brief Opaque bytes for a host-side ::lh_crypto_sha_t (≥ 216 on lh 0.4).
 */
#define AUTHUM_CRYPTO_SHA_STORAGE 256u

/**
 * @struct authum_crypto_sha
 * @brief Opaque SHA hasher. Treat as opaque; only use via crypto ops.
 */
struct authum_crypto_sha
{
    authum_usize_t storage[(AUTHUM_CRYPTO_SHA_STORAGE + sizeof(authum_usize_t) - 1u) /
                           sizeof(authum_usize_t)];
};
/** @copybrief authum_crypto_sha */
typedef struct authum_crypto_sha authum_crypto_sha_t;

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_CRYPTO_SHA_H */

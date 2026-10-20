/**
 * @file rijndael.h
 * @brief Rijndael session type for Authum modules (lh under the hood; no lh).
 *
 * Same idea as ::lh_crypto_rijndael_t: stack-allocated opaque session. Call
 * ::authum_crypto_rijndael_init / `_encrypt_block` / `_decrypt_block` from
 * <authum/crypto.h> (ops live on ::authum_host_t.crypto). Internal lh helpers
 * (gf_mul, ShiftRows, …) are not re-exported.
 */

#ifndef AUTHUM_CRYPTO_RIJNDAEL_H
#define AUTHUM_CRYPTO_RIJNDAEL_H

#include <authum/crypto/rijndael/mode.h>
#include <authum/size.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @def AUTHUM_CRYPTO_RIJNDAEL_BLOCK_SIZE_MIN
 * @brief Smallest supported block length in bytes (16 — AES).
 */
#define AUTHUM_CRYPTO_RIJNDAEL_BLOCK_SIZE_MIN 16u

/**
 * @def AUTHUM_CRYPTO_RIJNDAEL_BLOCK_SIZE_MAX
 * @brief Largest supported block length in bytes (32).
 */
#define AUTHUM_CRYPTO_RIJNDAEL_BLOCK_SIZE_MAX 32u

/**
 * @def AUTHUM_CRYPTO_RIJNDAEL_KEY_SIZE_MIN
 * @brief Smallest supported key length in bytes (16).
 */
#define AUTHUM_CRYPTO_RIJNDAEL_KEY_SIZE_MIN 16u

/**
 * @def AUTHUM_CRYPTO_RIJNDAEL_KEY_SIZE_MAX
 * @brief Largest supported key length in bytes (32).
 */
#define AUTHUM_CRYPTO_RIJNDAEL_KEY_SIZE_MAX 32u

/**
 * @def AUTHUM_CRYPTO_RIJNDAEL_STORAGE
 * @brief Opaque bytes for a host-side ::lh_crypto_rijndael_t (≥ 764 on lh 0.4).
 */
#define AUTHUM_CRYPTO_RIJNDAEL_STORAGE 768u

/**
 * @struct authum_crypto_rijndael
 * @brief Opaque Rijndael session. Treat as opaque; only use via crypto ops.
 */
struct authum_crypto_rijndael
{
    /** Word-aligned storage large enough for the host lh session. */
    authum_usize_t storage[(AUTHUM_CRYPTO_RIJNDAEL_STORAGE + sizeof(authum_usize_t) - 1u) /
                           sizeof(authum_usize_t)];
};
/** @copybrief authum_crypto_rijndael */
typedef struct authum_crypto_rijndael authum_crypto_rijndael_t;

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_CRYPTO_RIJNDAEL_H */

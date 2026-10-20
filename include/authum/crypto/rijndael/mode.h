/**
 * @file mode.h
 * @brief Block-cipher mode selector for ::authum_crypto_rijndael_t.
 *
 * Mirrors ::lh_crypto_rijndael_mode_* (subset exposed to modules; no lh).
 */

#ifndef AUTHUM_CRYPTO_RIJNDAEL_MODE_H
#define AUTHUM_CRYPTO_RIJNDAEL_MODE_H

#include <authum/byte.h>

/**
 * @def authum_crypto_rijndael_mode_ecb
 * @brief Electronic Code Book: each block is encrypted independently.
 */
#define authum_crypto_rijndael_mode_ecb 0

/**
 * @def authum_crypto_rijndael_mode_cbc
 * @brief Cipher Block Chaining.
 */
#define authum_crypto_rijndael_mode_cbc 1

/**
 * @def authum_crypto_rijndael_mode_cfb
 * @brief Cipher Feedback.
 */
#define authum_crypto_rijndael_mode_cfb 2

/**
 * @typedef authum_crypto_rijndael_mode_t
 * @brief One of the `authum_crypto_rijndael_mode_*` constants.
 */
typedef authum_byte_t authum_crypto_rijndael_mode_t;

#endif /* AUTHUM_CRYPTO_RIJNDAEL_MODE_H */

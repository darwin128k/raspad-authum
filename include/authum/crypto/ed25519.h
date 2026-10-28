/**
 * @file ed25519.h
 * @brief Ed25519 signatures (RFC 8032) for Authum modules.
 *
 * Unlike Rijndael / SHA, this is not a host facade: the code ships inside
 * `authum_client` (lh's `lh/crypto/ed25519.h`, Monocypher inside), so a module gets
 * it without a newer `authum` and without an ABI bump. Modules never
 * include `monocypher.h`.
 */

#ifndef AUTHUM_CRYPTO_ED25519_H
#define AUTHUM_CRYPTO_ED25519_H

#include <authum/bool.h>
#include <authum/extern.h>
#include <authum/ptr.h>
#include <authum/size.h>

AUTHUM_EXTERN_C_BEGIN

/**
 * @def AUTHUM_CRYPTO_ED25519_SEED_SIZE
 * @brief Random seed a key pair is derived from (32).
 */
#define AUTHUM_CRYPTO_ED25519_SEED_SIZE 32u

/**
 * @def AUTHUM_CRYPTO_ED25519_SECRET_KEY_SIZE
 * @brief Expanded secret key: seed followed by the public key (64).
 */
#define AUTHUM_CRYPTO_ED25519_SECRET_KEY_SIZE 64u

/**
 * @def AUTHUM_CRYPTO_ED25519_PUBLIC_KEY_SIZE
 * @brief Public key length in bytes (32).
 */
#define AUTHUM_CRYPTO_ED25519_PUBLIC_KEY_SIZE 32u

/**
 * @def AUTHUM_CRYPTO_ED25519_SIGNATURE_SIZE
 * @brief Signature length in bytes (64).
 */
#define AUTHUM_CRYPTO_ED25519_SIGNATURE_SIZE 64u

/**
 * Derive a key pair from @p seed. The seed is wiped afterwards — keep a
 * copy first if it has to be stored.
 */
void
authum_crypto_ed25519_key_pair(authum_byte_t secret_key[AUTHUM_CRYPTO_ED25519_SECRET_KEY_SIZE],
                               authum_byte_t public_key[AUTHUM_CRYPTO_ED25519_PUBLIC_KEY_SIZE],
                               authum_byte_t seed[AUTHUM_CRYPTO_ED25519_SEED_SIZE]);

/** Sign @p size bytes of @p message (deterministic, RFC 8032). */
void
authum_crypto_ed25519_sign(authum_byte_t signature[AUTHUM_CRYPTO_ED25519_SIGNATURE_SIZE],
                           const authum_byte_t secret_key[AUTHUM_CRYPTO_ED25519_SECRET_KEY_SIZE],
                           const authum_byte_t *message, authum_usize_t size);

/** True when @p signature over @p message verifies against @p public_key. */
authum_bool_t
authum_crypto_ed25519_check(const authum_byte_t signature[AUTHUM_CRYPTO_ED25519_SIGNATURE_SIZE],
                            const authum_byte_t public_key[AUTHUM_CRYPTO_ED25519_PUBLIC_KEY_SIZE],
                            const authum_byte_t *message, authum_usize_t size);

/** Overwrite @p size bytes at @p secret with zeros (not optimised away). */
void
authum_crypto_wipe(authum_ptr secret, authum_usize_t size);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_CRYPTO_ED25519_H */

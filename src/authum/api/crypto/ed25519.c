/**
 * @file ed25519.c
 * @brief ::authum_crypto_ed25519_* → ::lh_crypto_ed25519_* (Monocypher inside lh).
 */

#include <authum/crypto/ed25519.h>

#include <lh/crypto/ed25519.h>
#include <lh/crypto/wipe.h>

void
authum_crypto_ed25519_create_key_pair(
    authum_byte_t secret_key[AUTHUM_CRYPTO_ED25519_SECRET_KEY_SIZE],
    authum_byte_t public_key[AUTHUM_CRYPTO_ED25519_PUBLIC_KEY_SIZE],
    authum_byte_t seed[AUTHUM_CRYPTO_ED25519_SEED_SIZE])
{
    lh_crypto_ed25519_key_pair(secret_key, public_key, seed);
}

void
authum_crypto_ed25519_sign(authum_byte_t signature[AUTHUM_CRYPTO_ED25519_SIGNATURE_SIZE],
                           const authum_byte_t secret_key[AUTHUM_CRYPTO_ED25519_SECRET_KEY_SIZE],
                           const authum_byte_t *message, authum_usize_t size)
{
    lh_crypto_ed25519_sign(signature, secret_key, message, lh_cast_static(lh_usize_t, size));
}

authum_bool_t
authum_crypto_ed25519_is_valid(
    const authum_byte_t signature[AUTHUM_CRYPTO_ED25519_SIGNATURE_SIZE],
    const authum_byte_t public_key[AUTHUM_CRYPTO_ED25519_PUBLIC_KEY_SIZE],
    const authum_byte_t *message, authum_usize_t size)
{
    return lh_crypto_ed25519_check(signature, public_key, message, lh_cast_static(lh_usize_t, size))
               ? authum_bool_true
               : authum_bool_false;
}

void
authum_crypto_wipe(authum_ptr secret, authum_usize_t size)
{
    lh_crypto_wipe(secret, lh_cast_static(lh_usize_t, size));
}

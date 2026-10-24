#include <authum/crypto/ed25519.h>
#include <authum/null.h>

#include <monocypher-ed25519.h>
#include <monocypher.h>

void
authum_crypto_ed25519_key_pair(authum_byte_t secret_key[AUTHUM_CRYPTO_ED25519_SECRET_KEY_SIZE],
                               authum_byte_t public_key[AUTHUM_CRYPTO_ED25519_PUBLIC_KEY_SIZE],
                               authum_byte_t seed[AUTHUM_CRYPTO_ED25519_SEED_SIZE])
{
    if (authum_null_eq(secret_key) || authum_null_eq(public_key) || authum_null_eq(seed))
    {
        return;
    }
    crypto_ed25519_key_pair(secret_key, public_key, seed);
}

void
authum_crypto_ed25519_sign(authum_byte_t signature[AUTHUM_CRYPTO_ED25519_SIGNATURE_SIZE],
                           const authum_byte_t secret_key[AUTHUM_CRYPTO_ED25519_SECRET_KEY_SIZE],
                           const authum_byte_t *message, authum_usize_t size)
{
    if (authum_null_eq(signature) || authum_null_eq(secret_key) ||
        (authum_null_eq(message) && size > 0u))
    {
        return;
    }
    crypto_ed25519_sign(signature, secret_key, message, (size_t)size);
}

authum_bool_t
authum_crypto_ed25519_check(const authum_byte_t signature[AUTHUM_CRYPTO_ED25519_SIGNATURE_SIZE],
                            const authum_byte_t public_key[AUTHUM_CRYPTO_ED25519_PUBLIC_KEY_SIZE],
                            const authum_byte_t *message, authum_usize_t size)
{
    if (authum_null_eq(signature) || authum_null_eq(public_key) ||
        (authum_null_eq(message) && size > 0u))
    {
        return authum_bool_false;
    }
    return crypto_ed25519_check(signature, public_key, message, (size_t)size) == 0
               ? authum_bool_true
               : authum_bool_false;
}

void
authum_crypto_wipe(void *secret, authum_usize_t size)
{
    if (authum_null_eq(secret))
    {
        return;
    }
    crypto_wipe(secret, (size_t)size);
}

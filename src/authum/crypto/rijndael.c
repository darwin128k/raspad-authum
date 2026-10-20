/**
 * @file rijndael.c
 * @brief Host ::authum_crypto_t Rijndael ops → ::lh_crypto_rijndael_*.
 */

#include <authum/bool.h>
#include <authum/crypto/rijndael.h>
#include <lh/bool.h>
#include <lh/crypto/rijndael.h>
#include <lh/null.h>
#include <stddef.h>

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(lh_crypto_rijndael_t) <= AUTHUM_CRYPTO_RIJNDAEL_STORAGE,
               "AUTHUM_CRYPTO_RIJNDAEL_STORAGE too small for lh_crypto_rijndael_t");
#endif

static lh_crypto_rijndael_t *
authum_crypto_rijndael_lh(authum_crypto_rijndael_t *self)
{
    return (lh_crypto_rijndael_t *)(void *)self->storage;
}

authum_bool_t
authum_crypto_rijndael_init_impl(authum_crypto_rijndael_t *self, const void *key, size_t key_size,
                                 const void *chain, size_t chain_size, size_t block_size,
                                 authum_crypto_rijndael_mode_t mode)
{
    if (self == 0 || key == 0) {
        return AUTHUM_FALSE;
    }
    if (!lh_crypto_rijndael_init(authum_crypto_rijndael_lh(self), (lh_ptr)key, (lh_usize_t)key_size,
                                 (lh_ptr)chain, (lh_usize_t)chain_size, (lh_usize_t)block_size,
                                 (lh_crypto_rijndael_mode_t)mode)) {
        return AUTHUM_FALSE;
    }
    return AUTHUM_TRUE;
}

authum_bool_t
authum_crypto_rijndael_encrypt_block_impl(authum_crypto_rijndael_t *self, const void *in, void *out)
{
    if (self == 0 || in == 0 || out == 0) {
        return AUTHUM_FALSE;
    }
    if (!lh_crypto_rijndael_encrypt_block(authum_crypto_rijndael_lh(self), (lh_ptr)in,
                                          (lh_ptr)out)) {
        return AUTHUM_FALSE;
    }
    return AUTHUM_TRUE;
}

authum_bool_t
authum_crypto_rijndael_decrypt_block_impl(authum_crypto_rijndael_t *self, const void *in, void *out)
{
    if (self == 0 || in == 0 || out == 0) {
        return AUTHUM_FALSE;
    }
    if (!lh_crypto_rijndael_decrypt_block(authum_crypto_rijndael_lh(self), (lh_ptr)in,
                                          (lh_ptr)out)) {
        return AUTHUM_FALSE;
    }
    return AUTHUM_TRUE;
}

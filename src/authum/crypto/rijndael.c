/**
 * @file rijndael.c
 * @brief Host ::authum_crypto_t Rijndael ops → ::lh_crypto_rijndael_*.
 */

#include <authum/bool.h>
#include <authum/crypto/rijndael.h>
#include <authum/null.h>
#include <authum/ptr.h>
#include <authum/size.h>
#include <lh/bool.h>
#include <lh/crypto/rijndael.h>
#include <lh/null.h>

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(lh_crypto_rijndael_t) <= AUTHUM_CRYPTO_RIJNDAEL_STORAGE,
               "AUTHUM_CRYPTO_RIJNDAEL_STORAGE too small for lh_crypto_rijndael_t");
#endif

lh_crypto_rijndael_t *
authum_crypto_rijndael_lh(authum_crypto_rijndael_t *self)
{
    return (lh_crypto_rijndael_t *)(void *)self->storage;
}

authum_bool_t
authum_crypto_rijndael_init_impl(authum_crypto_rijndael_t *self, authum_ptr key,
                                 authum_usize_t key_size, authum_ptr chain, authum_usize_t chain_size,
                                 authum_usize_t block_size, authum_crypto_rijndael_mode_t mode)
{
    if (authum_null_eq(self) || authum_null_eq(key)) {
        return authum_bool_false;
    }
    if (!lh_crypto_rijndael_init(authum_crypto_rijndael_lh(self), (lh_ptr)key, (lh_usize_t)key_size,
                                 (lh_ptr)chain, (lh_usize_t)chain_size, (lh_usize_t)block_size,
                                 (lh_crypto_rijndael_mode_t)mode)) {
        return authum_bool_false;
    }
    return authum_bool_true;
}

authum_bool_t
authum_crypto_rijndael_encrypt_block_impl(authum_crypto_rijndael_t *self, authum_ptr in,
                                          authum_ptr out)
{
    if (authum_null_eq(self) || authum_null_eq(in) || authum_null_eq(out)) {
        return authum_bool_false;
    }
    if (!lh_crypto_rijndael_encrypt_block(authum_crypto_rijndael_lh(self), (lh_ptr)in,
                                          (lh_ptr)out)) {
        return authum_bool_false;
    }
    return authum_bool_true;
}

authum_bool_t
authum_crypto_rijndael_decrypt_block_impl(authum_crypto_rijndael_t *self, authum_ptr in,
                                          authum_ptr out)
{
    if (authum_null_eq(self) || authum_null_eq(in) || authum_null_eq(out)) {
        return authum_bool_false;
    }
    if (!lh_crypto_rijndael_decrypt_block(authum_crypto_rijndael_lh(self), (lh_ptr)in,
                                          (lh_ptr)out)) {
        return authum_bool_false;
    }
    return authum_bool_true;
}

/**
 * @file sha.c
 * @brief Host ::authum_crypto_t SHA ops → ::lh_crypto_sha_*.
 */

#include <authum/crypto/sha.h>
#include <authum/null.h>
#include <authum/ptr.h>
#include <authum/size.h>
#include <lh/crypto/sha.h>
#include <lh/ptr.h>

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(lh_crypto_sha_t) <= AUTHUM_CRYPTO_SHA_STORAGE,
               "AUTHUM_CRYPTO_SHA_STORAGE too small for lh_crypto_sha_t");
#endif

lh_crypto_sha_t *
authum_crypto_sha_lh(authum_crypto_sha_t *self)
{
    return (lh_crypto_sha_t *)(void *)self->storage;
}

void
authum_crypto_sha_init_impl(authum_crypto_sha_t *self, authum_crypto_sha_kind_t kind)
{
    if (authum_null_eq(self)) {
        return;
    }
    lh_crypto_sha_init(authum_crypto_sha_lh(self), (lh_crypto_sha_kind_t)kind);
}

void
authum_crypto_sha_update_impl(authum_crypto_sha_t *self, authum_ptr data, authum_usize_t size)
{
    if (authum_null_eq(self) || (authum_null_eq(data) && size != 0u)) {
        return;
    }
    lh_crypto_sha_update(authum_crypto_sha_lh(self), (lh_ptr)data, (lh_usize_t)size);
}

authum_usize_t
authum_crypto_sha_finish_impl(authum_crypto_sha_t *self, authum_ptr out, authum_usize_t out_size)
{
    if (authum_null_eq(self) || authum_null_eq(out)) {
        return 0u;
    }
    return (authum_usize_t)lh_crypto_sha_finish(authum_crypto_sha_lh(self), (lh_ptr)out,
                                                (lh_usize_t)out_size);
}

authum_usize_t
authum_crypto_sha_hash_impl(authum_crypto_sha_kind_t kind, authum_ptr data, authum_usize_t size,
                            authum_ptr out, authum_usize_t out_size)
{
    if (authum_null_eq(out) || (authum_null_eq(data) && size != 0u)) {
        return 0u;
    }
    return (authum_usize_t)lh_crypto_sha_hash((lh_crypto_sha_kind_t)kind, (lh_ptr)data,
                                              (lh_usize_t)size, (lh_ptr)out, (lh_usize_t)out_size);
}

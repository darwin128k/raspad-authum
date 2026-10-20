/**
 * @file sha.c
 * @brief Host ::authum_crypto_t SHA ops → ::lh_crypto_sha_*.
 */

#include <authum/crypto/sha.h>
#include <lh/crypto/sha.h>
#include <lh/ptr.h>
#include <stddef.h>

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(lh_crypto_sha_t) <= AUTHUM_CRYPTO_SHA_STORAGE,
               "AUTHUM_CRYPTO_SHA_STORAGE too small for lh_crypto_sha_t");
#endif

static lh_crypto_sha_t *
authum_crypto_sha_lh(authum_crypto_sha_t *self)
{
    return (lh_crypto_sha_t *)(void *)self->storage;
}

void
authum_crypto_sha_init_impl(authum_crypto_sha_t *self, authum_crypto_sha_kind_t kind)
{
    if (self == 0) {
        return;
    }
    lh_crypto_sha_init(authum_crypto_sha_lh(self), (lh_crypto_sha_kind_t)kind);
}

void
authum_crypto_sha_update_impl(authum_crypto_sha_t *self, const void *data, size_t size)
{
    if (self == 0 || (data == 0 && size != 0u)) {
        return;
    }
    lh_crypto_sha_update(authum_crypto_sha_lh(self), (lh_ptr)data, (lh_usize_t)size);
}

size_t
authum_crypto_sha_finish_impl(authum_crypto_sha_t *self, void *out, size_t out_size)
{
    if (self == 0 || out == 0) {
        return 0u;
    }
    return (size_t)lh_crypto_sha_finish(authum_crypto_sha_lh(self), (lh_ptr)out,
                                        (lh_usize_t)out_size);
}

size_t
authum_crypto_sha_hash_impl(authum_crypto_sha_kind_t kind, const void *data, size_t size, void *out,
                            size_t out_size)
{
    if (out == 0 || (data == 0 && size != 0u)) {
        return 0u;
    }
    return (size_t)lh_crypto_sha_hash((lh_crypto_sha_kind_t)kind, (lh_ptr)data, (lh_usize_t)size,
                                      (lh_ptr)out, (lh_usize_t)out_size);
}

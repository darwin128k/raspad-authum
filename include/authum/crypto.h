/**
 * @file crypto.h
 * @brief Host crypto facade for modules — lh layout, Authum names, no lh link.
 *
 * Umbrella for <authum/crypto/rijndael.h> and <authum/crypto/sha.h>. The host
 * fills ::authum_host_t.crypto with an ops table implemented inside
 * `authum_mm` (static lh). Modules call the wrappers below the same way they
 * would call `lh_crypto_*`, but pass the host crypto pointer.
 *
 * Not every lh crypto symbol is re-exported — only the session API modules need.
 */

#ifndef AUTHUM_CRYPTO_H
#define AUTHUM_CRYPTO_H

#include <authum/bool.h>
#include <authum/crypto/rijndael.h>
#include <authum/crypto/sha.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct authum_crypto
 * @brief Ops table: Rijndael + SHA subset, filled by the host from lh.
 */
typedef struct authum_crypto
{
    authum_bool_t (*rijndael_init)(authum_crypto_rijndael_t *self, const void *key, size_t key_size,
                                   const void *chain, size_t chain_size, size_t block_size,
                                   authum_crypto_rijndael_mode_t mode);
    authum_bool_t (*rijndael_encrypt_block)(authum_crypto_rijndael_t *self, const void *in,
                                            void *out);
    authum_bool_t (*rijndael_decrypt_block)(authum_crypto_rijndael_t *self, const void *in,
                                            void *out);

    void (*sha_init)(authum_crypto_sha_t *self, authum_crypto_sha_kind_t kind);
    void (*sha_update)(authum_crypto_sha_t *self, const void *data, size_t size);
    size_t (*sha_finish)(authum_crypto_sha_t *self, void *out, size_t out_size);
    size_t (*sha_hash)(authum_crypto_sha_kind_t kind, const void *data, size_t size, void *out,
                       size_t out_size);
} authum_crypto_t;

/** @see ::lh_crypto_rijndael_init */
static inline authum_bool_t
authum_crypto_rijndael_init(const authum_crypto_t *crypto, authum_crypto_rijndael_t *self,
                            const void *key, size_t key_size, const void *chain, size_t chain_size,
                            size_t block_size, authum_crypto_rijndael_mode_t mode)
{
    if (crypto == 0 || crypto->rijndael_init == 0) {
        return AUTHUM_FALSE;
    }
    return crypto->rijndael_init(self, key, key_size, chain, chain_size, block_size, mode);
}

/** @see ::lh_crypto_rijndael_encrypt_block */
static inline authum_bool_t
authum_crypto_rijndael_encrypt_block(const authum_crypto_t *crypto, authum_crypto_rijndael_t *self,
                                     const void *in, void *out)
{
    if (crypto == 0 || crypto->rijndael_encrypt_block == 0) {
        return AUTHUM_FALSE;
    }
    return crypto->rijndael_encrypt_block(self, in, out);
}

/** @see ::lh_crypto_rijndael_decrypt_block */
static inline authum_bool_t
authum_crypto_rijndael_decrypt_block(const authum_crypto_t *crypto, authum_crypto_rijndael_t *self,
                                     const void *in, void *out)
{
    if (crypto == 0 || crypto->rijndael_decrypt_block == 0) {
        return AUTHUM_FALSE;
    }
    return crypto->rijndael_decrypt_block(self, in, out);
}

/** @see ::lh_crypto_sha_init */
static inline void
authum_crypto_sha_init(const authum_crypto_t *crypto, authum_crypto_sha_t *self,
                       authum_crypto_sha_kind_t kind)
{
    if (crypto != 0 && crypto->sha_init != 0) {
        crypto->sha_init(self, kind);
    }
}

/** @see ::lh_crypto_sha_update */
static inline void
authum_crypto_sha_update(const authum_crypto_t *crypto, authum_crypto_sha_t *self, const void *data,
                         size_t size)
{
    if (crypto != 0 && crypto->sha_update != 0) {
        crypto->sha_update(self, data, size);
    }
}

/** @see ::lh_crypto_sha_finish */
static inline size_t
authum_crypto_sha_finish(const authum_crypto_t *crypto, authum_crypto_sha_t *self, void *out,
                         size_t out_size)
{
    if (crypto == 0 || crypto->sha_finish == 0) {
        return 0u;
    }
    return crypto->sha_finish(self, out, out_size);
}

/** @see ::lh_crypto_sha_hash */
static inline size_t
authum_crypto_sha_hash(const authum_crypto_t *crypto, authum_crypto_sha_kind_t kind,
                       const void *data, size_t size, void *out, size_t out_size)
{
    if (crypto == 0 || crypto->sha_hash == 0) {
        return 0u;
    }
    return crypto->sha_hash(kind, data, size, out, out_size);
}

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_CRYPTO_H */

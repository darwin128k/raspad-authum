/**
 * @file crypto.h
 * @brief Host crypto facade for modules — lh layout, Authum names, no lh link.
 *
 * Umbrella for <authum/crypto/rijndael.h> and <authum/crypto/sha.h>. The host
 * fills ::authum_host_t.crypto with an ops table implemented inside
 * `authum` (static lh). Modules call the wrappers below the same way they
 * would call `lh_crypto_*`, but pass the host crypto pointer.
 *
 * Not every lh crypto symbol is re-exported — only the session API modules need.
 */

#ifndef AUTHUM_CRYPTO_H
#define AUTHUM_CRYPTO_H

#include <authum/abi.h>
#include <authum/bool.h>
#include <authum/crypto/rijndael.h>
#include <authum/crypto/sha.h>
#include <authum/extern.h>
#include <authum/null.h>
#include <authum/ptr.h>
#include <authum/size.h>

AUTHUM_EXTERN_C_BEGIN

/**
 * @struct authum_crypto
 * @brief Ops table: Rijndael + SHA subset, filled by the host from lh.
 */
typedef struct authum_crypto
{
    /** sizeof this table as built by the host — see <authum/abi.h>. */
    authum_usize_t size;
    authum_bool_t (*rijndael_init)(authum_crypto_rijndael_t *self, authum_ptr key,
                                   authum_usize_t key_size, authum_ptr chain,
                                   authum_usize_t chain_size, authum_usize_t block_size,
                                   authum_crypto_rijndael_mode_t mode);
    authum_bool_t (*rijndael_encrypt_block)(authum_crypto_rijndael_t *self, authum_ptr in,
                                            authum_ptr out);
    authum_bool_t (*rijndael_decrypt_block)(authum_crypto_rijndael_t *self, authum_ptr in,
                                            authum_ptr out);

    void (*sha_init)(authum_crypto_sha_t *self, authum_crypto_sha_kind_t kind);
    void (*sha_update)(authum_crypto_sha_t *self, authum_ptr data, authum_usize_t size);
    authum_usize_t (*sha_finish)(authum_crypto_sha_t *self, authum_ptr out,
                                 authum_usize_t out_size);
    authum_usize_t (*sha_hash)(authum_crypto_sha_kind_t kind, authum_ptr data,
                               authum_usize_t size, authum_ptr out, authum_usize_t out_size);
} authum_crypto_t;

/** @see ::lh_crypto_rijndael_init */
authum_bool_t
authum_crypto_rijndael_init(const authum_crypto_t *crypto, authum_crypto_rijndael_t *self,
                            authum_ptr key, authum_usize_t key_size, authum_ptr chain,
                            authum_usize_t chain_size, authum_usize_t block_size,
                            authum_crypto_rijndael_mode_t mode);

/** @see ::lh_crypto_rijndael_encrypt_block */
authum_bool_t
authum_crypto_rijndael_encrypt_block(const authum_crypto_t *crypto,
                                     authum_crypto_rijndael_t *self, authum_ptr in,
                                     authum_ptr out);

/** @see ::lh_crypto_rijndael_decrypt_block */
authum_bool_t
authum_crypto_rijndael_decrypt_block(const authum_crypto_t *crypto,
                                     authum_crypto_rijndael_t *self, authum_ptr in,
                                     authum_ptr out);

/** @see ::lh_crypto_sha_init */
void
authum_crypto_sha_init(const authum_crypto_t *crypto, authum_crypto_sha_t *self,
                       authum_crypto_sha_kind_t kind);

/** @see ::lh_crypto_sha_update */
void
authum_crypto_sha_update(const authum_crypto_t *crypto, authum_crypto_sha_t *self,
                         authum_ptr data, authum_usize_t size);

/** @see ::lh_crypto_sha_finish */
authum_usize_t
authum_crypto_sha_finish(const authum_crypto_t *crypto, authum_crypto_sha_t *self,
                         authum_ptr out, authum_usize_t out_size);

/** @see ::lh_crypto_sha_hash */
authum_usize_t
authum_crypto_sha_hash(const authum_crypto_t *crypto, authum_crypto_sha_kind_t kind,
                       authum_ptr data, authum_usize_t size, authum_ptr out,
                       authum_usize_t out_size);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_CRYPTO_H */

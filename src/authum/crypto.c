/**
 * @file crypto.c
 * @brief Static ::authum_crypto_t ops table for ::authum_host_t.crypto.
 */

#include <authum/crypto.h>

authum_bool_t authum_crypto_rijndael_init_impl(authum_crypto_rijndael_t *self, const void *key,
                                               size_t key_size, const void *chain, size_t chain_size,
                                               size_t block_size, authum_crypto_rijndael_mode_t mode);
authum_bool_t authum_crypto_rijndael_encrypt_block_impl(authum_crypto_rijndael_t *self,
                                                        const void *in, void *out);
authum_bool_t authum_crypto_rijndael_decrypt_block_impl(authum_crypto_rijndael_t *self,
                                                        const void *in, void *out);

void authum_crypto_sha_init_impl(authum_crypto_sha_t *self, authum_crypto_sha_kind_t kind);
void authum_crypto_sha_update_impl(authum_crypto_sha_t *self, const void *data, size_t size);
size_t authum_crypto_sha_finish_impl(authum_crypto_sha_t *self, void *out, size_t out_size);
size_t authum_crypto_sha_hash_impl(authum_crypto_sha_kind_t kind, const void *data, size_t size,
                                   void *out, size_t out_size);

static const authum_crypto_t authum_crypto_ops = {
    .rijndael_init = authum_crypto_rijndael_init_impl,
    .rijndael_encrypt_block = authum_crypto_rijndael_encrypt_block_impl,
    .rijndael_decrypt_block = authum_crypto_rijndael_decrypt_block_impl,
    .sha_init = authum_crypto_sha_init_impl,
    .sha_update = authum_crypto_sha_update_impl,
    .sha_finish = authum_crypto_sha_finish_impl,
    .sha_hash = authum_crypto_sha_hash_impl,
};

const authum_crypto_t *
authum_crypto(void)
{
    return &authum_crypto_ops;
}

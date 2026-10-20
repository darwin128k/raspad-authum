/**
 * @file crypto.c
 * @brief Static ::authum_crypto_t ops table for ::authum_host_t.crypto.
 */

#include <authum/crypto.h>

authum_bool_t authum_crypto_rijndael_init_impl(authum_crypto_rijndael_t *self, authum_ptr key,
                                               authum_usize_t key_size, authum_ptr chain,
                                               authum_usize_t chain_size, authum_usize_t block_size,
                                               authum_crypto_rijndael_mode_t mode);
authum_bool_t authum_crypto_rijndael_encrypt_block_impl(authum_crypto_rijndael_t *self,
                                                        authum_ptr in, authum_ptr out);
authum_bool_t authum_crypto_rijndael_decrypt_block_impl(authum_crypto_rijndael_t *self,
                                                        authum_ptr in, authum_ptr out);

void authum_crypto_sha_init_impl(authum_crypto_sha_t *self, authum_crypto_sha_kind_t kind);
void authum_crypto_sha_update_impl(authum_crypto_sha_t *self, authum_ptr data, authum_usize_t size);
authum_usize_t authum_crypto_sha_finish_impl(authum_crypto_sha_t *self, authum_ptr out,
                                             authum_usize_t out_size);
authum_usize_t authum_crypto_sha_hash_impl(authum_crypto_sha_kind_t kind, authum_ptr data,
                                           authum_usize_t size, authum_ptr out,
                                           authum_usize_t out_size);

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

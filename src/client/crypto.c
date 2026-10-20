#include <authum/crypto.h>

authum_bool_t
authum_crypto_rijndael_init(const authum_crypto_t *crypto, authum_crypto_rijndael_t *self,
                            authum_ptr key, authum_usize_t key_size, authum_ptr chain,
                            authum_usize_t chain_size, authum_usize_t block_size,
                            authum_crypto_rijndael_mode_t mode)
{
    if (authum_null_eq(crypto) || crypto->rijndael_init == 0) {
        return authum_bool_false;
    }
    return crypto->rijndael_init(self, key, key_size, chain, chain_size, block_size, mode);
}

authum_bool_t
authum_crypto_rijndael_encrypt_block(const authum_crypto_t *crypto, authum_crypto_rijndael_t *self,
                                     authum_ptr in, authum_ptr out)
{
    if (authum_null_eq(crypto) || crypto->rijndael_encrypt_block == 0) {
        return authum_bool_false;
    }
    return crypto->rijndael_encrypt_block(self, in, out);
}

authum_bool_t
authum_crypto_rijndael_decrypt_block(const authum_crypto_t *crypto, authum_crypto_rijndael_t *self,
                                     authum_ptr in, authum_ptr out)
{
    if (authum_null_eq(crypto) || crypto->rijndael_decrypt_block == 0) {
        return authum_bool_false;
    }
    return crypto->rijndael_decrypt_block(self, in, out);
}

void
authum_crypto_sha_init(const authum_crypto_t *crypto, authum_crypto_sha_t *self,
                       authum_crypto_sha_kind_t kind)
{
    if (!authum_null_eq(crypto) && crypto->sha_init != 0) {
        crypto->sha_init(self, kind);
    }
}

void
authum_crypto_sha_update(const authum_crypto_t *crypto, authum_crypto_sha_t *self, authum_ptr data,
                         authum_usize_t size)
{
    if (!authum_null_eq(crypto) && crypto->sha_update != 0) {
        crypto->sha_update(self, data, size);
    }
}

authum_usize_t
authum_crypto_sha_finish(const authum_crypto_t *crypto, authum_crypto_sha_t *self, authum_ptr out,
                         authum_usize_t out_size)
{
    if (authum_null_eq(crypto) || crypto->sha_finish == 0) {
        return 0u;
    }
    return crypto->sha_finish(self, out, out_size);
}

authum_usize_t
authum_crypto_sha_hash(const authum_crypto_t *crypto, authum_crypto_sha_kind_t kind, authum_ptr data,
                       authum_usize_t size, authum_ptr out, authum_usize_t out_size)
{
    if (authum_null_eq(crypto) || crypto->sha_hash == 0) {
        return 0u;
    }
    return crypto->sha_hash(kind, data, size, out, out_size);
}

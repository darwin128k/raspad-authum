#include <authum/crypto/ed25519.h>
#include <authum/unused.h>
#include <authum/null.h>
#include <gtest/gtest.h>
#include <stdio.h>
#include <string.h>

void
test_hex(authum_byte_t *out, const char *hex)
{
    unsigned int byte;

    while (hex[0] != '\0' && hex[1] != '\0')
    {
        sscanf(hex, "%2x", &byte);
        *out++ = (authum_byte_t)byte;
        hex += 2;
    }
}

/* RFC 8032 section 7.1, TEST 1 (empty message) and TEST 2 (one byte 0x72). */
TEST(authum_crypto_ed25519, matches_rfc8032_vectors_and_rejects_changed_message)
{
    authum_byte_t seed[AUTHUM_CRYPTO_ED25519_SEED_SIZE];
    authum_byte_t secret_key[AUTHUM_CRYPTO_ED25519_SECRET_KEY_SIZE];
    authum_byte_t public_key[AUTHUM_CRYPTO_ED25519_PUBLIC_KEY_SIZE];
    authum_byte_t expected_key[AUTHUM_CRYPTO_ED25519_PUBLIC_KEY_SIZE];
    authum_byte_t signature[AUTHUM_CRYPTO_ED25519_SIGNATURE_SIZE];
    authum_byte_t expected_sig[AUTHUM_CRYPTO_ED25519_SIGNATURE_SIZE];
    authum_byte_t message[1] = {0x72};
    authum_byte_t zeros[AUTHUM_CRYPTO_ED25519_SEED_SIZE];

    test_hex(seed, "9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60");
    test_hex(expected_key, "d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a");
    test_hex(expected_sig, "e5564300c360ac729086e2cc806e828a84877f1eb8e5d974d873e06522490155"
                           "5fb8821590a33bacc61e39701cf9b46bd25bf5f0595bbe24655141438e7a100b");
    authum_crypto_ed25519_create_key_pair(secret_key, public_key, seed);
    memset(zeros, 0, sizeof zeros);
    EXPECT_EQ(memcmp(seed, zeros, sizeof zeros), 0);
    EXPECT_EQ(memcmp(public_key, expected_key, sizeof expected_key), 0);
    authum_crypto_ed25519_sign(signature, secret_key, authum_null, 0u);
    EXPECT_EQ(memcmp(signature, expected_sig, sizeof expected_sig), 0);
    EXPECT_TRUE(authum_crypto_ed25519_is_valid(signature, public_key, authum_null, 0u));

    test_hex(seed, "4ccd089b28ff96da9db6c346ec114e0f5b8a319f35aba624da8cf6ed4fb8a6fb");
    test_hex(expected_key, "3d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c");
    test_hex(expected_sig, "92a009a9f0d4cab8720e820b5f642540a2b27b5416503f8fb3762223ebdb69da"
                           "085ac1e43e15996e458f3613d0f11d8c387b2eaeb4302aeeb00d291612bb0c00");
    authum_crypto_ed25519_create_key_pair(secret_key, public_key, seed);
    EXPECT_EQ(memcmp(public_key, expected_key, sizeof expected_key), 0);
    authum_crypto_ed25519_sign(signature, secret_key, message, sizeof message);
    EXPECT_EQ(memcmp(signature, expected_sig, sizeof expected_sig), 0);
    EXPECT_TRUE(authum_crypto_ed25519_is_valid(signature, public_key, message, sizeof message));

    /* Any flipped bit in message, signature or key must fail. */
    message[0] ^= 1u;
    EXPECT_FALSE(authum_crypto_ed25519_is_valid(signature, public_key, message, sizeof message));
    message[0] ^= 1u;
    signature[0] ^= 1u;
    EXPECT_FALSE(authum_crypto_ed25519_is_valid(signature, public_key, message, sizeof message));
    signature[0] ^= 1u;
    public_key[0] ^= 1u;
    EXPECT_FALSE(authum_crypto_ed25519_is_valid(signature, public_key, message, sizeof message));
    public_key[0] ^= 1u;
    EXPECT_FALSE(authum_crypto_ed25519_is_valid(authum_null, public_key, message, sizeof message));

    authum_crypto_wipe(secret_key, sizeof secret_key);
    EXPECT_EQ(memcmp(secret_key, zeros, sizeof zeros), 0);
}

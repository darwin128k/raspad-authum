/**
 * @file client.c
 * @brief authum_client: cstr, path, cfg, engine kick, ed25519, detour, symbol.
 */

#include <check.h>

#include <authum/cfg.h>
#include <authum/crypto/ed25519.h>
#include <authum/cstr.h>
#include <authum/detour.h>
#include <authum/engine.h>
#include <authum/net.h>
#include <authum/path.h>
#include <authum/symbol.h>

#include <stdio.h>
#include <string.h>

void
test_cstr(void)
{
    authum_char_t buf[4];
    authum_i32_t i;
    authum_u32_t u;
    authum_bool_t b;

    CHECK(authum_cstr_copy(buf, sizeof buf, "abcdef"));
    CHECK_STR(buf, "abc");
    CHECK(authum_cstr_eq("Foo", "foo", authum_bool_true));
    CHECK(!authum_cstr_eq("Foo", "foo", authum_bool_false));
    CHECK(authum_cstr_parse_i32(" -42 ", &i) && i == -42);
    CHECK(authum_cstr_parse_i32("-2147483648", &i) && i == (-2147483647 - 1));
    CHECK(!authum_cstr_parse_i32("2147483648", &i));
    CHECK(!authum_cstr_parse_i32("12x", &i));
    CHECK(authum_cstr_parse_u32("4294967295", &u) && u == 4294967295u);
    CHECK(!authum_cstr_parse_u32("4294967296", &u));
    CHECK(authum_cstr_parse_bool("Yes", &b) && b);
    CHECK(authum_cstr_parse_bool("off", &b) && !b);
    CHECK(authum_cstr_parse_bool("7", &b) && b);
    CHECK(!authum_cstr_parse_bool("maybe", &b));
}

void
test_path(void)
{
    authum_path_t dir;
    authum_path_t out;

    CHECK(authum_path_set_cstr(&dir, "/srv/hlds/cstrike/addons"));
    CHECK(authum_path_join(&out, &dir, "authum"));
    CHECK_STR(authum_path_as_cstr(&out), "/srv/hlds/cstrike/addons/authum");
    CHECK(authum_path_set_cstr(&dir, "C:\\hlds\\"));
    CHECK(authum_path_join(&out, &dir, "x.cfg"));
    CHECK_STR(authum_path_as_cstr(&out), "C:\\hlds\\x.cfg");
    CHECK(authum_path_set_cstr(&dir, "a/b/c"));
    CHECK(authum_path_parent(&out, &dir));
    CHECK_STR(authum_path_as_cstr(&out), "a/b");
    CHECK(authum_path_set_cstr(&dir, "/a"));
    CHECK(authum_path_parent(&out, &dir));
    CHECK_STR(authum_path_as_cstr(&out), "/");
    CHECK(authum_path_set_cstr(&dir, "C:/a"));
    CHECK(authum_path_parent(&out, &dir));
    CHECK_STR(authum_path_as_cstr(&out), "C:/");
    authum_path_clear(&out);
    CHECK(authum_path_is_empty(&out));
    CHECK(authum_path_as_cstr_or_null(&out) == authum_null);
}

struct test_cfg
{
    authum_bool_t enable;
    authum_i32_t level;
    authum_u32_t mask;
    authum_char_t text[16];
};

void
test_cfg(void)
{
    struct test_cfg c = {authum_bool_false, 5, 1u, "default"};
    const authum_cfg_item_t items[] = {
        {"enable", authum_cfg_kind_bool, &c.enable, 0},
        {"level", authum_cfg_kind_i32, &c.level, 0},
        {"mask", authum_cfg_kind_u32, &c.mask, 0},
        {"text", authum_cfg_kind_cstr, c.text, sizeof c.text},
    };
    authum_cfg_t cfg;
    authum_char_t buf[32];
    FILE *f = tmpfile();

    CHECK(f != authum_null);
    if (f == authum_null)
    {
        return;
    }
    fputs("# comment\n; also comment\n\n  enable = yes \nlevel=not-a-number\n"
          "text = a=b=c\nextra = 12\n",
          f);
    rewind(f);
    CHECK(authum_cfg_adopt(&cfg, f));
    authum_cfg_apply_items(&cfg, items, sizeof items / sizeof items[0]);
    CHECK(c.enable);
    CHECK(c.level == 5);
    CHECK(c.mask == 1u);
    CHECK_STR(c.text, "a=b=c");
    CHECK(authum_cfg_get_i32(&cfg, "extra", -1) == 12);
    CHECK(authum_cfg_get_i32(&cfg, "missing", -1) == -1);
    CHECK(!authum_cfg_get_cstr(&cfg, "missing", buf, sizeof buf, "dflt"));
    CHECK_STR(buf, "dflt");
    CHECK(authum_cfg_get_cstr(&cfg, "text", buf, sizeof buf, "dflt"));
    CHECK_STR(buf, "a=b=c");
    authum_cfg_close(&cfg);
}

authum_char_t g_last_command[256];

void
test_engine_record_command(const authum_char_t *cmd)
{
    authum_cstr_copy(g_last_command, sizeof g_last_command, cmd);
}

void
test_engine_kick(void)
{
    authum_engine_t engine;

    memset(&engine, 0, sizeof engine);
    engine.server_command = test_engine_record_command;
    authum_engine_kick(&engine, 7, "bad \"guy\"; quit\n");
    CHECK_STR(g_last_command, "kick #7 \"bad  guy   quit \"\n");
    authum_engine_kick(&engine, 3, authum_null);
    CHECK_STR(g_last_command, "kick #3\n");
    authum_engine_kick(authum_null, 1, "x");
}

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
void
test_ed25519(void)
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
    authum_crypto_ed25519_key_pair(secret_key, public_key, seed);
    memset(zeros, 0, sizeof zeros);
    CHECK(memcmp(seed, zeros, sizeof zeros) == 0);
    CHECK(memcmp(public_key, expected_key, sizeof expected_key) == 0);
    authum_crypto_ed25519_sign(signature, secret_key, authum_null, 0u);
    CHECK(memcmp(signature, expected_sig, sizeof expected_sig) == 0);
    CHECK(authum_crypto_ed25519_check(signature, public_key, authum_null, 0u));

    test_hex(seed, "4ccd089b28ff96da9db6c346ec114e0f5b8a319f35aba624da8cf6ed4fb8a6fb");
    test_hex(expected_key, "3d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c");
    test_hex(expected_sig, "92a009a9f0d4cab8720e820b5f642540a2b27b5416503f8fb3762223ebdb69da"
                           "085ac1e43e15996e458f3613d0f11d8c387b2eaeb4302aeeb00d291612bb0c00");
    authum_crypto_ed25519_key_pair(secret_key, public_key, seed);
    CHECK(memcmp(public_key, expected_key, sizeof expected_key) == 0);
    authum_crypto_ed25519_sign(signature, secret_key, message, sizeof message);
    CHECK(memcmp(signature, expected_sig, sizeof expected_sig) == 0);
    CHECK(authum_crypto_ed25519_check(signature, public_key, message, sizeof message));

    /* Any flipped bit in message, signature or key must fail. */
    message[0] ^= 1u;
    CHECK(!authum_crypto_ed25519_check(signature, public_key, message, sizeof message));
    message[0] ^= 1u;
    signature[0] ^= 1u;
    CHECK(!authum_crypto_ed25519_check(signature, public_key, message, sizeof message));
    signature[0] ^= 1u;
    public_key[0] ^= 1u;
    CHECK(!authum_crypto_ed25519_check(signature, public_key, message, sizeof message));
    public_key[0] ^= 1u;
    CHECK(!authum_crypto_ed25519_check(authum_null, public_key, message, sizeof message));

    authum_crypto_wipe(secret_key, sizeof secret_key);
    CHECK(memcmp(secret_key, zeros, sizeof zeros) == 0);
}

authum_detour_t g_test_detour;

/* noinline + calls through a volatile pointer: the patched bytes must be what runs. */
__attribute__((noinline)) int
test_detour_target(int x)
{
    return x + 1;
}

int
test_detour_hook(int x)
{
    int original;

    authum_detour_suspend(&g_test_detour);
    original = ((int (*)(int))g_test_detour.target)(x);
    authum_detour_resume(&g_test_detour);
    return original * 100;
}

void
test_detour(void)
{
    int (*volatile fn)(int) = test_detour_target;
    authum_detour_t other;

    CHECK(fn(1) == 2);
#if defined(__i386__)
    CHECK(authum_detour_attach(&g_test_detour, (authum_ptr)fn, (authum_ptr)&test_detour_hook));
    CHECK(!authum_detour_attach(&g_test_detour, (authum_ptr)fn, (authum_ptr)&test_detour_hook));
    CHECK(fn(1) == 200);
    CHECK(fn(4) == 500);
    authum_detour_suspend(&g_test_detour);
    CHECK(fn(1) == 2);
    authum_detour_resume(&g_test_detour);
    CHECK(fn(1) == 200);
    authum_detour_detach(&g_test_detour);
    CHECK(fn(1) == 2);
    authum_detour_detach(&g_test_detour); /* second detach is a no-op */
    CHECK(fn(1) == 2);
#endif
    memset(&other, 0, sizeof other);
    CHECK(!authum_detour_attach(&other, authum_null, (authum_ptr)&test_detour_hook));
    CHECK(!authum_detour_attach(authum_null, (authum_ptr)fn, (authum_ptr)&test_detour_hook));
}

void
test_symbol(void)
{
    authum_bool_t via_dynsym = authum_bool_false;

    CHECK(authum_symbol_find("libc.so.6", "strlen", &via_dynsym) != authum_null);
    CHECK(via_dynsym);
    CHECK(authum_symbol_find("libc.so.6", "authum_no_such_symbol", &via_dynsym) == authum_null);
    CHECK(!via_dynsym);
    CHECK(authum_symbol_find("authum_not_loaded.so", "strlen", authum_null) == authum_null);
    CHECK(authum_symbol_find_engine("SVC_Info", authum_null) == authum_null);
    CHECK(authum_symbol_find(authum_null, "strlen", authum_null) == authum_null);
}

void
test_net(void)
{
    authum_char_t text[AUTHUM_NET_IPV4_TEXT_SIZE];
    authum_u32_t ips[8];
    authum_usize_t n;
    authum_usize_t i;

    CHECK(authum_net_ipv4_is_external(0x25e6d2dau));  /* 37.230.210.218 */
    CHECK(authum_net_ipv4_is_external(0x08080808u));  /* 8.8.8.8 */
    CHECK(authum_net_ipv4_is_external(0xac200001u));  /* 172.32.0.1, just past 172.16/12 */
    CHECK(!authum_net_ipv4_is_external(0x7f000001u)); /* 127.0.0.1 */
    CHECK(!authum_net_ipv4_is_external(0x0a000001u)); /* 10.0.0.1 */
    CHECK(!authum_net_ipv4_is_external(0xac1f0001u)); /* 172.31.0.1 */
    CHECK(!authum_net_ipv4_is_external(0xc0a80101u)); /* 192.168.1.1 */
    CHECK(!authum_net_ipv4_is_external(0x64400001u)); /* 100.64.0.1 CGNAT */
    CHECK(!authum_net_ipv4_is_external(0xa9fe0101u)); /* 169.254.1.1 */
    CHECK(!authum_net_ipv4_is_external(0xe0000001u)); /* 224.0.0.1 */
    CHECK(!authum_net_ipv4_is_external(0x00000000u));
    CHECK(authum_net_ipv4_is_loopback(0x7f000001u));
    CHECK(!authum_net_ipv4_is_loopback(0x0a000001u));

    CHECK(authum_net_ipv4_format(0x25e6d2dau, text, sizeof text));
    CHECK_STR(text, "37.230.210.218");
    CHECK(authum_net_ipv4_format(0xffffffffu, text, sizeof text));
    CHECK_STR(text, "255.255.255.255");
    CHECK(!authum_net_ipv4_format(0x25e6d2dau, text, 8u));

    /* Real interfaces: whatever this machine has, the lists must be clean. */
    n = authum_net_ipv4_external(ips, 8u);
    for (i = 0u; i < n && i < 8u; i++)
    {
        CHECK(authum_net_ipv4_is_external(ips[i]));
    }
    n = authum_net_ipv4_internal(ips, 8u);
    for (i = 0u; i < n && i < 8u; i++)
    {
        CHECK(!authum_net_ipv4_is_external(ips[i]) && !authum_net_ipv4_is_loopback(ips[i]));
    }
    CHECK(authum_net_ipv4_internal(authum_null, 0u) == n); /* count without a buffer */
}

int
main(void)
{
    test_net();
    test_cstr();
    test_path();
    test_cfg();
    test_engine_kick();
    test_ed25519();
    test_detour();
    test_symbol();
    return authum_check_report("authum_client");
}

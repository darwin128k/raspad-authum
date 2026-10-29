#include <authum/net.h>
#include <authum/unused.h>
#include <authum/null.h>
#include <gtest/gtest.h>
#include <stdio.h>
#include <string.h>

TEST(authum_net, classifies_external_addresses)
{
    const authum_u32_t addresses[] = {0x25e6d2dau, 0x08080808u, 0xac200001u};
    for (auto address : addresses)
    {
        EXPECT_TRUE(authum_net_ipv4_is_external(address)) << address;
    }
}

TEST(authum_net, excludes_reserved_and_private_addresses)
{
    const authum_u32_t addresses[] = {0x7f000001u, 0x0a000001u, 0xac1f0001u, 0xc0a80101u,
                                      0x64400001u, 0xa9fe0101u, 0xe0000001u, 0u};
    for (auto address : addresses)
    {
        EXPECT_FALSE(authum_net_ipv4_is_external(address)) << address;
    }
}

TEST(authum_net, recognizes_loopback)
{
    EXPECT_TRUE(authum_net_ipv4_is_loopback(0x7f000001u));
    EXPECT_FALSE(authum_net_ipv4_is_loopback(0x0a000001u));
}

TEST(authum_net, formats_ipv4_octets)
{
    authum_char_t text[AUTHUM_NET_IPV4_TEXT_SIZE];
    ASSERT_TRUE(authum_net_ipv4_format(0x25e6d2dau, text, sizeof text));
    EXPECT_STREQ(text, "37.230.210.218");
    ASSERT_TRUE(authum_net_ipv4_format(0xffffffffu, text, sizeof text));
    EXPECT_STREQ(text, "255.255.255.255");
}

TEST(authum_net, formatting_refuses_insufficient_capacity)
{
    authum_char_t text[8];
    EXPECT_FALSE(authum_net_ipv4_format(0x25e6d2dau, text, sizeof text));
}

TEST(authum_net, collected_interfaces_obey_requested_address_class)
{
    authum_u32_t ips[8];
    auto n = authum_net_ipv4_collect_external(ips, 8u);
    for (authum_usize_t i = 0; i < n && i < 8; ++i)
    {
        EXPECT_TRUE(authum_net_ipv4_is_external(ips[i]));
    }
    n = authum_net_ipv4_collect_internal(ips, 8u);
    for (authum_usize_t i = 0; i < n && i < 8; ++i)
    {
        EXPECT_FALSE(authum_net_ipv4_is_external(ips[i]) && !authum_net_ipv4_is_loopback(ips[i]));
    }
    EXPECT_EQ(authum_net_ipv4_collect_internal(authum_null, 0u), n);
}

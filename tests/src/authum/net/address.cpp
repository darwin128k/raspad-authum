#include <authum/net/address.h>
#include <authum/null.h>
#include <gtest/gtest.h>
#include <tuple>

using address_case = std::tuple<const char *, const char *>;

class authum_net_address_valid : public ::testing::TestWithParam<address_case>
{
};

TEST_P(authum_net_address_valid, copies_host_without_resolving)
{
    char out[64];
    const auto value = GetParam();
    ASSERT_TRUE(authum_net_address_copy_host(std::get<0>(value), out, sizeof out));
    EXPECT_STREQ(out, std::get<1>(value));
}

INSTANTIATE_TEST_SUITE_P(host_forms, authum_net_address_valid,
                         ::testing::Values(address_case{"192.0.2.10:27015", "192.0.2.10"},
                                           address_case{"example.org", "example.org"},
                                           address_case{"2001:db8::1", "2001:db8::1"}));

class authum_net_address_invalid : public ::testing::TestWithParam<const char *>
{
};

TEST_P(authum_net_address_invalid, clears_output_on_invalid_address)
{
    char out[64] = "sentinel";
    EXPECT_FALSE(authum_net_address_copy_host(GetParam(), out, sizeof out));
    EXPECT_STREQ(out, "");
}

INSTANTIATE_TEST_SUITE_P(invalid_addresses, authum_net_address_invalid,
                         ::testing::Values("[::1", "a:65536", "a:abc", "a\r\nb"));

TEST(authum_net_address, supports_in_place_extraction)
{
    char address[64] = "[2001:db8::1]:27015";
    ASSERT_TRUE(authum_net_address_copy_host(address, address, sizeof address));
    EXPECT_STREQ(address, "2001:db8::1");
}

TEST(authum_net_address, clears_output_when_buffer_is_too_small)
{
    char out[64] = "sentinel";
    EXPECT_FALSE(authum_net_address_copy_host("host:1", out, 2));
    EXPECT_STREQ(out, "");
}

TEST(authum_net_address, refuses_null_input)
{
    char out[64];
    EXPECT_FALSE(authum_net_address_copy_host(authum_null, out, sizeof out));
}

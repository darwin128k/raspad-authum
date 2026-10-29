#include <authum/cstr.h>
#include <authum/unused.h>
#include <gtest/gtest.h>
#include <stdio.h>
#include <string.h>

TEST(authum_cstr, copy_truncates_to_capacity)
{
    authum_char_t buf[4];
    authum_i32_t i;
    authum_u32_t u;
    authum_bool_t b;
    EXPECT_TRUE(authum_cstr_copy(buf, sizeof buf, "abcdef"));
    EXPECT_STREQ(buf, "abc");
}

TEST(authum_cstr, comparison_obeys_case_policy)
{
    authum_char_t buf[4];
    authum_i32_t i;
    authum_u32_t u;
    authum_bool_t b;
    EXPECT_TRUE(authum_cstr_is_equal("Foo", "foo", authum_bool_true));
    EXPECT_FALSE(authum_cstr_is_equal("Foo", "foo", authum_bool_false));
}

TEST(authum_cstr, signed_parser_enforces_range_and_complete_input)
{
    authum_char_t buf[4];
    authum_i32_t i;
    authum_u32_t u;
    authum_bool_t b;
    EXPECT_TRUE(authum_cstr_parse_i32(" -42 ", &i) && i == -42);
    EXPECT_TRUE(authum_cstr_parse_i32("-2147483648", &i) && i == (-2147483647 - 1));
    EXPECT_FALSE(authum_cstr_parse_i32("2147483648", &i));
    EXPECT_FALSE(authum_cstr_parse_i32("12x", &i));
}

TEST(authum_cstr, unsigned_parser_enforces_range)
{
    authum_char_t buf[4];
    authum_i32_t i;
    authum_u32_t u;
    authum_bool_t b;
    EXPECT_TRUE(authum_cstr_parse_u32("4294967295", &u) && u == 4294967295u);
    EXPECT_FALSE(authum_cstr_parse_u32("4294967296", &u));
}

TEST(authum_cstr, signed_parser_accepts_sign_and_zero_padding)
{
    authum_char_t buf[4];
    authum_i32_t i;
    authum_u32_t u;
    authum_bool_t b;
    EXPECT_TRUE(authum_cstr_parse_i32("+5", &i) && i == 5);
    EXPECT_TRUE(authum_cstr_parse_i32("007", &i) && i == 7); /* zero padding is fine */
    EXPECT_TRUE(authum_cstr_parse_i32("-0", &i) && i == 0);
}

TEST(authum_cstr, signed_parser_refuses_missing_or_separated_digits)
{
    authum_char_t buf[4];
    authum_i32_t i;
    authum_u32_t u;
    authum_bool_t b;
    EXPECT_FALSE(authum_cstr_parse_i32("-", &i));
    EXPECT_FALSE(authum_cstr_parse_i32("", &i));
    EXPECT_FALSE(authum_cstr_parse_i32("  ", &i));
    EXPECT_FALSE(authum_cstr_parse_i32("- 5", &i));
}

TEST(authum_cstr, unsigned_parser_accepts_whitespace_but_not_negative_sign)
{
    authum_char_t buf[4];
    authum_i32_t i;
    authum_u32_t u;
    authum_bool_t b;
    EXPECT_TRUE(authum_cstr_parse_u32("\t+7 ", &u) && u == 7u);
    EXPECT_FALSE(authum_cstr_parse_u32("-1", &u));
}

TEST(authum_cstr, boolean_parser_accepts_known_spellings)
{
    authum_char_t buf[4];
    authum_i32_t i;
    authum_u32_t u;
    authum_bool_t b;
    EXPECT_TRUE(authum_cstr_parse_bool("Yes", &b) && b);
    EXPECT_TRUE(authum_cstr_parse_bool("off", &b) && !b);
    EXPECT_TRUE(authum_cstr_parse_bool("7", &b) && b);
    EXPECT_FALSE(authum_cstr_parse_bool("maybe", &b));
}

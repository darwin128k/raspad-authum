#include <authum/cstr.h>
#include <authum/unused.h>
#include <gtest/gtest.h>
#include <stdarg.h>
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

/** The va_list twin of ::authum_cstr_format, for the test below. */
static authum_usize_t
format_v(authum_char_t *dst, authum_usize_t dst_size, const authum_char_t *fmt, ...)
{
    va_list args;
    authum_usize_t n;

    va_start(args, fmt);
    n = authum_cstr_format_v(dst, dst_size, fmt, args);
    va_end(args);
    return n;
}

TEST(authum_cstr, format_writes_the_supported_conversions)
{
    authum_char_t buf[32];
    authum_i32_t i;
    authum_u32_t u;
    authum_bool_t b;
    EXPECT_EQ(authum_cstr_format(buf, sizeof buf, "%s|%d|%u|%x|%X|%c|%%", "ab", -7, 42u, 255u, 255u,
                                 'Z'),
              18u);
    EXPECT_STREQ(buf, "ab|-7|42|ff|FF|Z|%");
}

TEST(authum_cstr, format_cuts_a_result_that_does_not_fit)
{
    authum_char_t buf[4];
    authum_i32_t i;
    authum_u32_t u;
    authum_bool_t b;
    EXPECT_EQ(authum_cstr_format(buf, sizeof buf, "abcdef"), 3u);
    EXPECT_STREQ(buf, "abc");
}

TEST(authum_cstr, format_refuses_an_unknown_conversion)
{
    authum_char_t buf[8];
    authum_i32_t i;
    authum_u32_t u;
    authum_bool_t b;
    EXPECT_EQ(authum_cstr_format(buf, sizeof buf, "%q"), 0u);
    EXPECT_STREQ(buf, "");
}

TEST(authum_cstr, format_v_writes_the_same_text)
{
    authum_char_t buf[16];
    authum_i32_t i;
    authum_u32_t u;
    authum_bool_t b;
    EXPECT_EQ(format_v(buf, sizeof buf, "%d of %u", -7, 42u), 8u);
    EXPECT_STREQ(buf, "-7 of 42");
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

#include <authum/cstr.h>
#include <gtest/gtest.h>

TEST(authum_char, digits)
{
    EXPECT_TRUE(authum_char_is_digit('0'));
    EXPECT_TRUE(authum_char_is_digit('9'));
    EXPECT_FALSE(authum_char_is_digit('a'));
    EXPECT_FALSE(authum_char_is_digit(' '));
}

TEST(authum_char, letters)
{
    EXPECT_TRUE(authum_char_is_letter('a'));
    EXPECT_TRUE(authum_char_is_letter('Z'));
    EXPECT_FALSE(authum_char_is_letter('7'));
    EXPECT_TRUE(authum_char_is_lower('a'));
    EXPECT_FALSE(authum_char_is_lower('A'));
    EXPECT_TRUE(authum_char_is_upper('A'));
    EXPECT_FALSE(authum_char_is_upper('a'));
}

TEST(authum_char, space)
{
    EXPECT_TRUE(authum_char_is_space(' '));
    EXPECT_TRUE(authum_char_is_space('\t'));
    EXPECT_FALSE(authum_char_is_space('a'));
}

TEST(authum_char, hex_digits)
{
    EXPECT_TRUE(authum_char_is_xdigit('0'));
    EXPECT_TRUE(authum_char_is_xdigit('f'));
    EXPECT_TRUE(authum_char_is_xdigit('A'));
    EXPECT_FALSE(authum_char_is_xdigit('g'));
}

TEST(authum_char, to_upper)
{
    EXPECT_EQ(authum_char_to_upper('a'), 'A');
    EXPECT_EQ(authum_char_to_upper('z'), 'Z');
    EXPECT_EQ(authum_char_to_upper('7'), '7');
}

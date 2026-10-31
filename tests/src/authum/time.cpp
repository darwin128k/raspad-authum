#include <authum/time.h>
#include <authum/cstr.h>
#include <gtest/gtest.h>

TEST(authum_time, now_is_a_plausible_moment)
{
    authum_i64_t now = authum_time_now_seconds();

    EXPECT_NE(now, 0);
    EXPECT_GT(now, 1600000000);
}

TEST(authum_time, formats_the_epoch)
{
    authum_char_t text[32];

    EXPECT_EQ(authum_time_format_utc(0, text, sizeof text), 23u);
    EXPECT_STREQ(text, "1970-01-01 00:00:00 UTC");
}

TEST(authum_time, formats_a_known_moment)
{
    authum_char_t text[32];

    EXPECT_EQ(authum_time_format_utc(1700000000, text, sizeof text), 23u);
    EXPECT_STREQ(text, "2023-11-14 22:13:20 UTC");
}

TEST(authum_time, refuses_a_buffer_without_room)
{
    authum_char_t text[32];

    EXPECT_EQ(authum_time_format_utc(0, text, 4u), 0u);
    EXPECT_EQ(authum_time_format_utc(0, authum_null, 32u), 0u);
}

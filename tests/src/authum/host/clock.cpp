#include <authum/host/clock.h>
#include <authum/null.h>
#include <gtest/gtest.h>
#include <unistd.h>

TEST(authum_clock, monotonic_time_does_not_move_backwards)
{
    const auto *clock = authum_clock_host();
    const auto before = authum_clock_get_monotonic_ms(clock);
    ASSERT_GT(before, 0);
    usleep(2000);
    EXPECT_GE(authum_clock_get_monotonic_ms(clock), before);
}

TEST(authum_clock, missing_service_returns_zero)
{
    EXPECT_EQ(authum_clock_get_monotonic_ms(authum_null), 0);
}

TEST(authum_clock, missing_callback_returns_zero)
{
    authum_clock_t clock = {.monotonic_ms = authum_null};
    EXPECT_EQ(authum_clock_get_monotonic_ms(&clock), 0);
}

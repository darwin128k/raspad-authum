#include <authum/host.h>
#include <authum/unused.h>
#include <gtest/gtest.h>
#include <stdio.h>
#include <string.h>

TEST(authum_host, copies_and_clears_facade)
{
    authum_host_t src;
    authum_host_t dst;
    authum_engine_t engine;

    memset(&engine, 0, sizeof engine);
    memset(&src, 0, sizeof src);
    src.engine = &engine;
    src.commands = (const authum_commands_t *)&engine; /* any non-null marker */

    authum_host_copy(&dst, &src);
    EXPECT_EQ(authum_host_get_engine(&dst), &engine);
    EXPECT_EQ(authum_host_get_commands(&dst), src.commands);

    authum_host_clear(&dst);
    EXPECT_EQ(authum_host_get_engine(&dst), authum_null);
    EXPECT_EQ(authum_host_get_commands(&dst), authum_null);

    memset(&dst, 0xff, sizeof dst);
    authum_host_copy(&dst, authum_null);
    EXPECT_EQ(authum_host_get_engine(&dst), authum_null); /* zeroed, not garbage */
}

#include <authum/detour.h>
#include <authum/unused.h>
#include <gtest/gtest.h>
#include <stdio.h>
#include <string.h>

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

TEST(authum_detour, installs_suspends_resumes_and_removes_hook)
{
    int (*volatile fn)(int) = test_detour_target;
    authum_detour_t other;

    EXPECT_EQ(fn(1), 2);
#if defined(__i386__)
    EXPECT_TRUE(
        authum_detour_attach(&g_test_detour, (authum_ptr)fn, (authum_ptr)&test_detour_hook));
    EXPECT_FALSE(
        authum_detour_attach(&g_test_detour, (authum_ptr)fn, (authum_ptr)&test_detour_hook));
    EXPECT_EQ(fn(1), 200);
    EXPECT_EQ(fn(4), 500);
    authum_detour_suspend(&g_test_detour);
    EXPECT_EQ(fn(1), 2);
    authum_detour_resume(&g_test_detour);
    EXPECT_EQ(fn(1), 200);
    authum_detour_detach(&g_test_detour);
    EXPECT_EQ(fn(1), 2);
    authum_detour_detach(&g_test_detour); /* second detach is a no-op */
    EXPECT_EQ(fn(1), 2);
#endif
    memset(&other, 0, sizeof other);
    EXPECT_FALSE(authum_detour_attach(&other, authum_null, (authum_ptr)&test_detour_hook));
    EXPECT_FALSE(authum_detour_attach(authum_null, (authum_ptr)fn, (authum_ptr)&test_detour_hook));
}

#include <authum/version.h>
#include <authum/unused.h>
#include <gtest/gtest.h>
#include <stdio.h>
#include <string.h>

TEST(authum_version, accepts_later_host_minor)
{
    const authum_version_t v2_0 = AUTHUM_VERSION(2, 0, 0);
    const authum_version_t v2_0_7 = AUTHUM_VERSION(2, 0, 7);
    const authum_version_t v2_1 = AUTHUM_VERSION(2, 1, 0);
    const authum_version_t v1_5 = AUTHUM_VERSION(1, 5, 0);
    const authum_version_t v3_0 = AUTHUM_VERSION(3, 0, 0);

    EXPECT_TRUE(authum_version_is_abi_compatible(&v2_0, &v2_1));
}

TEST(authum_version, rejects_older_host_minor)
{
    const authum_version_t v2_0 = AUTHUM_VERSION(2, 0, 0);
    const authum_version_t v2_0_7 = AUTHUM_VERSION(2, 0, 7);
    const authum_version_t v2_1 = AUTHUM_VERSION(2, 1, 0);
    const authum_version_t v1_5 = AUTHUM_VERSION(1, 5, 0);
    const authum_version_t v3_0 = AUTHUM_VERSION(3, 0, 0);

    EXPECT_FALSE(authum_version_is_abi_compatible(&v2_1, &v2_0));
}

TEST(authum_version, ignores_patch_version)
{
    const authum_version_t v2_0 = AUTHUM_VERSION(2, 0, 0);
    const authum_version_t v2_0_7 = AUTHUM_VERSION(2, 0, 7);
    const authum_version_t v2_1 = AUTHUM_VERSION(2, 1, 0);
    const authum_version_t v1_5 = AUTHUM_VERSION(1, 5, 0);
    const authum_version_t v3_0 = AUTHUM_VERSION(3, 0, 0);

    EXPECT_TRUE(authum_version_is_abi_compatible(&v2_0_7, &v2_0));
}

TEST(authum_version, rejects_older_major)
{
    const authum_version_t v2_0 = AUTHUM_VERSION(2, 0, 0);
    const authum_version_t v2_0_7 = AUTHUM_VERSION(2, 0, 7);
    const authum_version_t v2_1 = AUTHUM_VERSION(2, 1, 0);
    const authum_version_t v1_5 = AUTHUM_VERSION(1, 5, 0);
    const authum_version_t v3_0 = AUTHUM_VERSION(3, 0, 0);

    EXPECT_FALSE(authum_version_is_abi_compatible(&v1_5, &v2_0));
}

TEST(authum_version, rejects_newer_major)
{
    const authum_version_t v2_0 = AUTHUM_VERSION(2, 0, 0);
    const authum_version_t v2_0_7 = AUTHUM_VERSION(2, 0, 7);
    const authum_version_t v2_1 = AUTHUM_VERSION(2, 1, 0);
    const authum_version_t v1_5 = AUTHUM_VERSION(1, 5, 0);
    const authum_version_t v3_0 = AUTHUM_VERSION(3, 0, 0);

    EXPECT_FALSE(authum_version_is_abi_compatible(&v3_0, &v2_1));
}

TEST(authum_version, rejects_missing_module_version)
{
    const authum_version_t v2_0 = AUTHUM_VERSION(2, 0, 0);
    const authum_version_t v2_0_7 = AUTHUM_VERSION(2, 0, 7);
    const authum_version_t v2_1 = AUTHUM_VERSION(2, 1, 0);
    const authum_version_t v1_5 = AUTHUM_VERSION(1, 5, 0);
    const authum_version_t v3_0 = AUTHUM_VERSION(3, 0, 0);

    EXPECT_FALSE(authum_version_is_abi_compatible(authum_null, &v2_0));
}

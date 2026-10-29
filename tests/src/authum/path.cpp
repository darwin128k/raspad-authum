#include <authum/path.h>
#include <authum/unused.h>
#include <gtest/gtest.h>
#include <stdio.h>
#include <string.h>

TEST(authum_path, joins_unix_path)
{
    authum_path_t dir;
    authum_path_t out;
    EXPECT_TRUE(authum_path_set_cstr(&dir, "/srv/hlds/cstrike/addons"));
    EXPECT_TRUE(authum_path_join(&out, &dir, "authum"));
    EXPECT_STREQ(authum_path_get_cstr(&out), "/srv/hlds/cstrike/addons/authum");
}

TEST(authum_path, preserves_windows_separator)
{
    authum_path_t dir;
    authum_path_t out;
    EXPECT_TRUE(authum_path_set_cstr(&dir, "C:\\hlds\\"));
    EXPECT_TRUE(authum_path_join(&out, &dir, "x.cfg"));
    EXPECT_STREQ(authum_path_get_cstr(&out), "C:\\hlds\\x.cfg");
}

TEST(authum_path, assigns_relative_parent)
{
    authum_path_t dir;
    authum_path_t out;
    EXPECT_TRUE(authum_path_set_cstr(&dir, "a/b/c"));
    EXPECT_TRUE(authum_path_assign_parent(&out, &dir));
    EXPECT_STREQ(authum_path_get_cstr(&out), "a/b");
}

TEST(authum_path, preserves_unix_root_parent)
{
    authum_path_t dir;
    authum_path_t out;
    EXPECT_TRUE(authum_path_set_cstr(&dir, "/a"));
    EXPECT_TRUE(authum_path_assign_parent(&out, &dir));
    EXPECT_STREQ(authum_path_get_cstr(&out), "/");
}

TEST(authum_path, preserves_drive_root_parent)
{
    authum_path_t dir;
    authum_path_t out;
    EXPECT_TRUE(authum_path_set_cstr(&dir, "C:/a"));
    EXPECT_TRUE(authum_path_assign_parent(&out, &dir));
    EXPECT_STREQ(authum_path_get_cstr(&out), "C:/");
}

TEST(authum_path, clear_produces_empty_path)
{
    authum_path_t dir;
    authum_path_t out;
    authum_path_clear(&out);
    EXPECT_TRUE(authum_path_is_empty(&out));
    EXPECT_EQ(authum_path_get_cstr_or_null(&out), authum_null);
}

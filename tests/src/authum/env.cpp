#include <authum/env.h>
#include <authum/host.h>
#include <authum/unused.h>
#include <gtest/gtest.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#if defined(_WIN32)
#    include <direct.h>
#endif

/* --- configs/ and data/ ---------------------------------------------------- */

#define TEST_ENV_ROOT "authum_env_test_root"

void
test_env_touch(const char *path, const char *text)
{
    FILE *file = fopen(path, "w");

    if (file != authum_null)
    {
        fputs(text, file);
        fclose(file);
    }
}

authum_bool_t
test_env_exists(const char *path)
{
    FILE *file = fopen(path, "rb");

    if (file == authum_null)
    {
        return authum_bool_false;
    }
    fclose(file);
    return authum_bool_true;
}

TEST(authum_env, separates_config_and_data_lifetimes)
{
    authum_path_t root, module, module_config, module_data, out;
    authum_env_t env;
    struct stat info;

    authum_path_set_cstr(&root, TEST_ENV_ROOT);
    authum_path_join(&module, &root, "foo");
    authum_path_join(&module_config, &module, "configs");
    authum_path_join(&module_data, &module, "data");
    EXPECT_TRUE(authum_path_ensure_dir(&root) && authum_path_ensure_dir(&module) &&
                authum_path_ensure_dir(&module_config));
    remove(TEST_ENV_ROOT "/foo/configs/foo.cfg");

    memset(&env, 0, sizeof env);
    env.module = &module;
    env.module_config = &module_config;
    env.module_data = &module_data;

    /* configs/<leaf>: the path either way, true once the file is there. */
    EXPECT_FALSE(authum_env_join_config_path(&env, "foo.cfg", &out));
    EXPECT_STREQ(authum_path_get_cstr(&out), TEST_ENV_ROOT "/foo/configs/foo.cfg");
    test_env_touch(TEST_ENV_ROOT "/foo/configs/foo.cfg", "x\n");
    EXPECT_TRUE(authum_env_join_config_path(&env, "foo.cfg", &out));

    remove(TEST_ENV_ROOT "/foo/data/foo.db");
#if defined(_WIN32)
    _rmdir(TEST_ENV_ROOT "/foo/data");
#else
    remove(TEST_ENV_ROOT "/foo/data");
#endif
    EXPECT_NE(stat(TEST_ENV_ROOT "/foo/data", &info), 0);
    /* Constructing a path for a read must not create the directory. */
    EXPECT_TRUE(authum_env_join_data_path(&env, "foo.db", &out));
    EXPECT_STREQ(authum_path_get_cstr(&out), TEST_ENV_ROOT "/foo/data/foo.db");
    EXPECT_NE(stat(TEST_ENV_ROOT "/foo/data", &info), 0);
    EXPECT_TRUE(authum_path_ensure_dir(&module_data));
    test_env_touch(TEST_ENV_ROOT "/foo/data/foo.db", "db\n");
    EXPECT_TRUE(test_env_exists(TEST_ENV_ROOT "/foo/data/foo.db"));

    /* Bad input: no paths, no crash. */
    EXPECT_FALSE(authum_env_join_config_path(&env, "", &out));
    EXPECT_FALSE(authum_env_join_data_path(&env, authum_null, &out));
    EXPECT_FALSE(authum_env_join_config_path(authum_null, "foo.cfg", &out));
}

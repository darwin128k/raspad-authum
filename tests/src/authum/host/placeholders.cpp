#ifndef _GNU_SOURCE
#    define _GNU_SOURCE
#endif
#include <authum/host/placeholders.h>

#include <gtest/gtest.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static authum_char_t game_dir[512];
static authum_char_t module_dir[512];

void
setup(void)
{
    authum_char_t base[384];
    ASSERT_NE(getcwd(base, sizeof base), authum_null);
    snprintf(game_dir, sizeof game_dir, "%s/authum-placeholders-game", base);
    snprintf(module_dir, sizeof module_dir, "%s/authum-placeholders-module", base);
#if defined(_WIN32)
    mkdir(game_dir);
    mkdir(module_dir);
#else
    mkdir(game_dir, 0755);
    mkdir(module_dir, 0755);
#endif
    ASSERT_TRUE(authum_placeholder_canonical(game_dir, game_dir, sizeof game_dir));
    ASSERT_TRUE(authum_placeholder_canonical(module_dir, module_dir, sizeof module_dir));
}

void
teardown(void)
{
    rmdir(game_dir);
    rmdir(module_dir);
}

static authum_placeholder_owner_t owner, other;
static authum_placeholder_service_t service, other_service;
static authum_char_t out[512];

namespace tests
{

class authum_placeholders : public ::testing::Test
{
  protected:
    void
    SetUp() override
    {
        ASSERT_NO_FATAL_FAILURE(setup());
        const authum_char_t *standard[authum_placeholder_standard_count] = {0};
        standard[authum_placeholder_game] = game_dir;
        standard[authum_placeholder_module] = module_dir;
        authum_placeholder_host_init(&owner, standard, &service);
        authum_placeholder_host_init(&other, standard, &other_service);
        out[0] = 0;
    }

    void
    TearDown() override
    {
        authum_placeholder_host_drop_owner(&owner);
        authum_placeholder_host_drop_owner(&other);
        teardown();
    }
};

TEST_F(authum_placeholders, canonicalizes_existing_directory)
{
    authum_char_t dotted[640];
    snprintf(dotted, sizeof dotted, "%s/.", game_dir);
    ASSERT_TRUE(authum_placeholder_canonical(dotted, out, sizeof out));
    EXPECT_STREQ(out, game_dir);
}

TEST_F(authum_placeholders, preserves_unresolved_path_including_in_place)
{
    const authum_char_t *missing = "authum-placeholders-missing/../future";
    ASSERT_TRUE(authum_placeholder_canonical(missing, out, sizeof out));
    EXPECT_STREQ(out, missing);
    ASSERT_TRUE(authum_placeholder_canonical(out, out, sizeof out));
    EXPECT_STREQ(out, missing);
}

TEST_F(authum_placeholders, canonicalization_preserves_output_on_overflow)
{
    strcpy(out, "sentinel");
    EXPECT_FALSE(authum_placeholder_canonical(game_dir, out, 2));
    EXPECT_STREQ(out, "sentinel");
    EXPECT_FALSE(authum_placeholder_canonical("missing/future", out, 2));
    EXPECT_STREQ(out, "sentinel");
}

TEST_F(authum_placeholders, canonicalization_rejects_invalid_arguments)
{
    EXPECT_FALSE(authum_placeholder_canonical(authum_null, out, sizeof out));
    EXPECT_FALSE(authum_placeholder_canonical(game_dir, authum_null, sizeof out));
    EXPECT_FALSE(authum_placeholder_canonical(game_dir, out, 0));
}

TEST_F(authum_placeholders, expands_standard_names)
{
    ASSERT_TRUE(authum_placeholder_expand(&service, "{game}", out, sizeof out));
    EXPECT_STREQ(out, game_dir);
    ASSERT_TRUE(authum_placeholder_expand(&service, "{module}", out, sizeof out));
    EXPECT_STREQ(out, module_dir);
    ASSERT_TRUE(authum_placeholder_expand(&service, "{game}/maps", out, sizeof out));
    EXPECT_NE(strstr(out, "/maps"), authum_null);
}

TEST_F(authum_placeholders, resolves_relative_template_against_module)
{
    ASSERT_TRUE(authum_placeholder_expand(&service, "maps", out, sizeof out));
    EXPECT_NE(strstr(out, module_dir), authum_null);
}

class authum_placeholder_invalid_template
    : public authum_placeholders,
      public ::testing::WithParamInterface<const authum_char_t *>
{
};

TEST_P(authum_placeholder_invalid_template, rejects_invalid_template)
{
    EXPECT_FALSE(authum_placeholder_expand(&service, GetParam(), out, sizeof out));
}

INSTANTIATE_TEST_SUITE_P(invalid_templates, authum_placeholder_invalid_template,
                         ::testing::Values("{unknown}", "{game", "game}", ""));

TEST_F(authum_placeholders, expansion_rejects_output_overflow)
{
    EXPECT_FALSE(authum_placeholder_expand(&service, "{game}", out, 4));
}

TEST_F(authum_placeholders, publishes_registered_name_to_other_owners)
{
    ASSERT_TRUE(authum_placeholder_register(&service, "shared", "/tmp/authum-placeholders-shared"));
    ASSERT_TRUE(authum_placeholder_expand(&service, "{shared}", out, sizeof out));
    EXPECT_NE(strstr(out, "authum-placeholders-shared"), authum_null);
    ASSERT_TRUE(authum_placeholder_expand(&other_service, "{shared}", out, sizeof out));
    EXPECT_NE(strstr(out, "authum-placeholders-shared"), authum_null);
}

TEST_F(authum_placeholders, refuses_reserved_and_duplicate_names)
{
    EXPECT_FALSE(authum_placeholder_register(&service, "game", "/tmp"));
    ASSERT_TRUE(authum_placeholder_register(&service, "shared", "/tmp/authum-placeholders-shared"));
    EXPECT_FALSE(authum_placeholder_register(&service, "shared", "/tmp"));
    ASSERT_TRUE(authum_placeholder_expand(&service, "{shared}", out, sizeof out));
    EXPECT_NE(strstr(out, "authum-placeholders-shared"), authum_null);
}

TEST_F(authum_placeholders, only_registering_owner_can_remove_name)
{
    ASSERT_TRUE(authum_placeholder_register(&service, "shared", "/tmp/authum-placeholders-shared"));
    EXPECT_FALSE(authum_placeholder_unregister(&other_service, "shared"));
    ASSERT_TRUE(authum_placeholder_expand(&other_service, "{shared}", out, sizeof out));
    EXPECT_TRUE(authum_placeholder_unregister(&service, "shared"));
    EXPECT_FALSE(authum_placeholder_expand(&service, "{shared}", out, sizeof out));
}

TEST_F(authum_placeholders, unloading_owner_drops_published_names)
{
    ASSERT_TRUE(authum_placeholder_register(&service, "gone", "/tmp/authum-placeholders-gone"));
    ASSERT_TRUE(authum_placeholder_expand(&other_service, "{gone}", out, sizeof out));
    authum_placeholder_host_drop_owner(&owner);
    EXPECT_FALSE(authum_placeholder_expand(&other_service, "{gone}", out, sizeof out));
}

} // namespace tests

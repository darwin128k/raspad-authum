#include <authum/cfg.h>
#include <authum/unused.h>
#include <gtest/gtest.h>
#include <string.h>

static authum_bool_t enable, adopted;
static authum_i32_t level;
static authum_u32_t mask;
static authum_char_t text[16];
static authum_cfg_t cfg;
static const authum_char_t config[] = "# comment\n; also comment\n\n  enable = yes \nlevel=not-a-number\n"
                                      "text = a=b=c\nextra = 12\n";
static const authum_cfg_item_t items[] = {{"enable", authum_cfg_kind_bool, &enable, 0},
                                          {"level", authum_cfg_kind_i32, &level, 0},
                                          {"mask", authum_cfg_kind_u32, &mask, 0},
                                          {"text", authum_cfg_kind_cstr, text, sizeof text}};

namespace tests
{
class authum_cfg : public ::testing::Test
{
  protected:
    void
    SetUp() override
    {
        enable = adopted = authum_bool_false;
        level = 5;
        mask = 1;
        strcpy(text, "default");
        adopted = authum_cfg_open_text(&cfg, config, sizeof config - 1);
        ASSERT_TRUE(adopted);
    }

    void
    TearDown() override
    {
        if (adopted)
        {
            authum_cfg_close(&cfg);
        }
    }
};

TEST_F(authum_cfg, applies_present_boolean)
{
    authum_cfg_apply_items(&cfg, items, sizeof items / sizeof items[0]);
    EXPECT_TRUE(enable);
}

TEST_F(authum_cfg, invalid_number_keeps_default)
{
    authum_cfg_apply_items(&cfg, items, sizeof items / sizeof items[0]);
    EXPECT_EQ(level, 5);
}

TEST_F(authum_cfg, missing_key_keeps_default)
{
    authum_cfg_apply_items(&cfg, items, sizeof items / sizeof items[0]);
    EXPECT_EQ(mask, 1u);
}

TEST_F(authum_cfg, value_preserves_additional_equals)
{
    authum_cfg_apply_items(&cfg, items, sizeof items / sizeof items[0]);
    EXPECT_STREQ(text, "a=b=c");
}

TEST_F(authum_cfg, integer_lookup_uses_explicit_fallback)
{
    EXPECT_EQ(authum_cfg_get_i32(&cfg, "extra", -1), 12);
    EXPECT_EQ(authum_cfg_get_i32(&cfg, "missing", -1), -1);
}

TEST_F(authum_cfg, missing_string_copies_fallback_and_reports_absence)
{
    authum_char_t out[32];
    EXPECT_FALSE(authum_cfg_copy_cstr(&cfg, "missing", out, sizeof out, "dflt"));
    EXPECT_STREQ(out, "dflt");
}

TEST_F(authum_cfg, found_string_copies_value)
{
    authum_char_t out[32];
    ASSERT_TRUE(authum_cfg_copy_cstr(&cfg, "text", out, sizeof out, "dflt"));
    EXPECT_STREQ(out, "a=b=c");
}
} // namespace tests

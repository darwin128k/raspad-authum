/**
 * @file client.c
 * @brief authum_client: cstr, path, cfg, engine kick.
 */

#include <check.h>

#include <authum/cfg.h>
#include <authum/cstr.h>
#include <authum/engine.h>
#include <authum/path.h>

#include <stdio.h>
#include <string.h>

void
test_cstr(void)
{
    authum_char_t buf[4];
    authum_i32_t i;
    authum_u32_t u;
    authum_bool_t b;

    CHECK(authum_cstr_copy(buf, sizeof buf, "abcdef"));
    CHECK_STR(buf, "abc");
    CHECK(authum_cstr_eq("Foo", "foo", authum_bool_true));
    CHECK(!authum_cstr_eq("Foo", "foo", authum_bool_false));
    CHECK(authum_cstr_parse_i32(" -42 ", &i) && i == -42);
    CHECK(authum_cstr_parse_i32("-2147483648", &i) && i == (-2147483647 - 1));
    CHECK(!authum_cstr_parse_i32("2147483648", &i));
    CHECK(!authum_cstr_parse_i32("12x", &i));
    CHECK(authum_cstr_parse_u32("4294967295", &u) && u == 4294967295u);
    CHECK(!authum_cstr_parse_u32("4294967296", &u));
    CHECK(authum_cstr_parse_bool("Yes", &b) && b);
    CHECK(authum_cstr_parse_bool("off", &b) && !b);
    CHECK(authum_cstr_parse_bool("7", &b) && b);
    CHECK(!authum_cstr_parse_bool("maybe", &b));
}

void
test_path(void)
{
    authum_path_t dir;
    authum_path_t out;

    CHECK(authum_path_set_cstr(&dir, "/srv/hlds/cstrike/addons"));
    CHECK(authum_path_join(&out, &dir, "authum"));
    CHECK_STR(authum_path_as_cstr(&out), "/srv/hlds/cstrike/addons/authum");
    CHECK(authum_path_set_cstr(&dir, "C:\\hlds\\"));
    CHECK(authum_path_join(&out, &dir, "x.cfg"));
    CHECK_STR(authum_path_as_cstr(&out), "C:\\hlds\\x.cfg");
    CHECK(authum_path_set_cstr(&dir, "a/b/c"));
    CHECK(authum_path_parent(&out, &dir));
    CHECK_STR(authum_path_as_cstr(&out), "a/b");
    CHECK(authum_path_set_cstr(&dir, "/a"));
    CHECK(authum_path_parent(&out, &dir));
    CHECK_STR(authum_path_as_cstr(&out), "/");
    CHECK(authum_path_set_cstr(&dir, "C:/a"));
    CHECK(authum_path_parent(&out, &dir));
    CHECK_STR(authum_path_as_cstr(&out), "C:/");
    authum_path_clear(&out);
    CHECK(authum_path_is_empty(&out));
    CHECK(authum_path_as_cstr_or_null(&out) == authum_null);
}

struct test_cfg
{
    authum_bool_t enable;
    authum_i32_t level;
    authum_u32_t mask;
    authum_char_t text[16];
};

void
test_cfg(void)
{
    struct test_cfg c = { authum_bool_false, 5, 1u, "default" };
    const authum_cfg_item_t items[] = {
        { "Enable", authum_cfg_kind_bool, &c.enable, 0 },
        { "Level", authum_cfg_kind_i32, &c.level, 0 },
        { "Mask", authum_cfg_kind_u32, &c.mask, 0 },
        { "Text", authum_cfg_kind_cstr, c.text, sizeof c.text },
    };
    authum_cfg_t cfg;
    authum_char_t buf[32];
    FILE *f = tmpfile();

    CHECK(f != authum_null);
    if (f == authum_null) {
        return;
    }
    fputs("# comment\n; also comment\n\n  enable = yes \nLevel=not-a-number\n"
          "Text = a=b=c\nExtra = 12\n",
          f);
    rewind(f);
    CHECK(authum_cfg_adopt(&cfg, f));
    authum_cfg_apply_items(&cfg, items, sizeof items / sizeof items[0]);
    CHECK(c.enable);
    CHECK(c.level == 5);
    CHECK(c.mask == 1u);
    CHECK_STR(c.text, "a=b=c");
    CHECK(authum_cfg_get_i32(&cfg, "extra", -1) == 12);
    CHECK(authum_cfg_get_i32(&cfg, "missing", -1) == -1);
    CHECK(!authum_cfg_get_cstr(&cfg, "missing", buf, sizeof buf, "dflt"));
    CHECK_STR(buf, "dflt");
    CHECK(authum_cfg_get_cstr(&cfg, "text", buf, sizeof buf, "dflt"));
    CHECK_STR(buf, "a=b=c");
    authum_cfg_close(&cfg);
}

authum_char_t g_last_command[256];

void
test_engine_record_command(const authum_char_t *cmd)
{
    authum_cstr_copy(g_last_command, sizeof g_last_command, cmd);
}

void
test_engine_kick(void)
{
    authum_engine_t engine;

    memset(&engine, 0, sizeof engine);
    engine.server_command = test_engine_record_command;
    authum_engine_kick(&engine, 7, "bad \"guy\"; quit\n");
    CHECK_STR(g_last_command, "kick #7 \"bad  guy   quit \"\n");
    authum_engine_kick(&engine, 3, authum_null);
    CHECK_STR(g_last_command, "kick #3\n");
    authum_engine_kick(authum_null, 1, "x");
}

int
main(void)
{
    test_cstr();
    test_path();
    test_cfg();
    test_engine_kick();
    return authum_check_report("authum_client");
}

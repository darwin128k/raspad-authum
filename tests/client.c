/**
 * @file client.c
 * @brief authum_client: cstr, path, cfg, engine kick/cvar query, ed25519, detour, symbol, json,
 * sql.
 */

#include <check.h>

#include <authum/cfg.h>
#include <authum/crypto/ed25519.h>
#include <authum/cstr.h>
#include <authum/detour.h>
#include <authum/engine.h>
#include <authum/event/message.h>
#include <authum/host.h>
#include <authum/json.h>
#include <authum/net.h>
#include <authum/path.h>
#include <authum/sql.h>
#include <authum/symbol.h>
#include <authum/version.h>

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
    struct test_cfg c = {authum_bool_false, 5, 1u, "default"};
    const authum_cfg_item_t items[] = {
        {"enable", authum_cfg_kind_bool, &c.enable, 0},
        {"level", authum_cfg_kind_i32, &c.level, 0},
        {"mask", authum_cfg_kind_u32, &c.mask, 0},
        {"text", authum_cfg_kind_cstr, c.text, sizeof c.text},
    };
    authum_cfg_t cfg;
    authum_char_t buf[32];
    FILE *f = tmpfile();

    CHECK(f != authum_null);
    if (f == authum_null)
    {
        return;
    }
    fputs("# comment\n; also comment\n\n  enable = yes \nlevel=not-a-number\n"
          "text = a=b=c\nextra = 12\n",
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
    engine.size = sizeof engine;
    engine.server_command = test_engine_record_command;
    authum_engine_kick(&engine, 7, "bad \"guy\"; quit\n");
    CHECK_STR(g_last_command, "kick #7 \"bad  guy   quit \"\n");
    authum_engine_kick(&engine, 3, authum_null);
    CHECK_STR(g_last_command, "kick #3\n");
    authum_engine_kick(authum_null, 1, "x");
}

authum_i32_t g_last_cvar_index;
authum_i32_t g_last_cvar_request;
authum_char_t g_last_cvar[64];

authum_bool_t
test_engine_record_cvar(authum_i32_t index, const authum_char_t *cvar, authum_i32_t request_id)
{
    g_last_cvar_index = index;
    g_last_cvar_request = request_id;
    authum_cstr_copy(g_last_cvar, sizeof g_last_cvar, cvar);
    return index == 2 ? authum_bool_true : authum_bool_false;
}

void
test_engine_query_cvar(void)
{
    authum_engine_t engine;

    memset(&engine, 0, sizeof engine);
    engine.size = sizeof engine;
    CHECK(!authum_engine_query_cvar(&engine, 2, "rate", 1));
    engine.query_client_cvar = test_engine_record_cvar;
    CHECK(authum_engine_query_cvar(&engine, 2, "cl_updaterate", 42));
    CHECK(g_last_cvar_index == 2);
    CHECK(g_last_cvar_request == 42);
    CHECK_STR(g_last_cvar, "cl_updaterate");
    CHECK(!authum_engine_query_cvar(&engine, 5, "rate", 1));
    CHECK(!authum_engine_query_cvar(&engine, 2, "", 1));
    CHECK(!authum_engine_query_cvar(&engine, 2, authum_null, 1));
    CHECK(!authum_engine_query_cvar(authum_null, 2, "rate", 1));
}

/* --- user messages and cvars through a fake engine ------------------------ */

authum_char_t g_msg_log[256];

void
test_msg_log(const authum_char_t *text)
{
    authum_usize_t used = strlen(g_msg_log);

    snprintf(g_msg_log + used, sizeof g_msg_log - used, "%s;", text);
}

authum_i32_t
test_msg_id(const authum_char_t *name)
{
    return strcmp(name, "SayText") == 0 ? 76 : 0;
}

authum_bool_t
test_msg_begin(authum_i32_t dest, authum_i32_t msg_id, authum_i32_t index)
{
    authum_char_t line[64];

    if (index == 9)
    {
        return authum_bool_false; /* nobody in slot 9 */
    }
    snprintf(line, sizeof line, "begin %d %d %d", (int)dest, (int)msg_id, (int)index);
    test_msg_log(line);
    return authum_bool_true;
}

void
test_msg_write(authum_i32_t kind, authum_i32_t value, authum_float_t real,
               const authum_char_t *text)
{
    authum_char_t line[96];

    snprintf(line, sizeof line, "w%d %d %.2f %s", (int)kind, (int)value, (double)real,
             text != NULL ? text : "-");
    test_msg_log(line);
}

void
test_msg_end(void)
{
    test_msg_log("end");
}

authum_bool_t
test_cvar_get(const authum_char_t *name, authum_char_t *buf, authum_usize_t buf_size)
{
    const authum_char_t *value = strcmp(name, "a") == 0   ? "12"
                                 : strcmp(name, "b") == 0 ? "0.25"
                                 : strcmp(name, "c") == 0 ? "x"
                                                          : NULL;

    if (value == NULL)
    {
        return authum_bool_false;
    }
    authum_cstr_copy(buf, buf_size, value);
    return authum_bool_true;
}

void
test_engine_messages(void)
{
    authum_engine_t engine;

    memset(&engine, 0, sizeof engine);
    engine.size = sizeof engine;
    engine.message_id = test_msg_id;
    engine.message_begin = test_msg_begin;
    engine.message_write = test_msg_write;
    engine.message_end = test_msg_end;
    engine.cvar_get = test_cvar_get;

    g_msg_log[0] = '\0';
    CHECK(authum_engine_client_chat(&engine, 3, 3, "hi"));
    CHECK_STR(g_msg_log, "begin 1 76 3;w0 3 0.00 -;w6 0 0.00 hi;end;");

    g_msg_log[0] = '\0';
    CHECK(authum_engine_client_chat(&engine, 0, 0, "all")); /* everyone: dest all */
    CHECK_STR(g_msg_log, "begin 2 76 0;w0 0 0.00 -;w6 0 0.00 all;end;");

    /* A refused begin: the writes and the end after it go nowhere. */
    g_msg_log[0] = '\0';
    CHECK(!authum_engine_client_chat(&engine, 9, 0, "x"));
    CHECK(!authum_engine_message_begin(&engine, authum_message_dest_one, 76, 9));
    authum_engine_message_write_coord(&engine, 1.5f);
    authum_engine_message_end(&engine);
    CHECK_STR(g_msg_log, "");

    g_msg_log[0] = '\0';
    CHECK(authum_engine_message_begin(&engine, authum_message_dest_all, 76, 0));
    authum_engine_message_write_coord(&engine, 1.5f);
    authum_engine_message_write_string(&engine, authum_null);
    authum_engine_message_end(&engine);
    CHECK_STR(g_msg_log, "begin 2 76 0;w5 0 1.50 -;w6 0 0.00 ;end;");

    CHECK(authum_engine_message_id(&engine, "DeathMsg") == 0);
    CHECK(!authum_engine_message_begin(&engine, authum_message_dest_all, 0, 0)); /* no id */
    engine.message_id = 0;
    CHECK(!authum_engine_client_chat(&engine, 1, 1, "no SayText")); /* host without it */

    CHECK(authum_engine_cvar_get_i32(&engine, "a", 5) == 12);
    CHECK(authum_engine_cvar_get_i32(&engine, "c", 5) == 5); /* not a number */
    CHECK(authum_engine_cvar_get_i32(&engine, "missing", 5) == 5);
    CHECK(authum_engine_cvar_get_float(&engine, "b", 1.0f) == 0.25f);
    CHECK(authum_engine_cvar_get_float(&engine, "c", 1.0f) == 1.0f);
}

/* --- entities through a fake engine: slot 1 alive, 2 dead, 3 a bot ---------- */

authum_float_t g_ent_health[4] = {0.0f, 100.0f, 0.0f, 35.0f};
authum_i32_t g_ent_dead[4] = {0, authum_entity_dead_no, authum_entity_dead_dead,
                              authum_entity_dead_no};
authum_i32_t g_ent_flags[4] = {0, authum_entity_flag_client, authum_entity_flag_client,
                               authum_entity_flag_client | authum_entity_flag_fake_client};

authum_bool_t
test_ent_connected(authum_i32_t index)
{
    return index >= 1 && index <= 3 ? authum_bool_true : authum_bool_false;
}

authum_bool_t
test_ent_get_float(authum_i32_t index, authum_i32_t field, authum_float_t *out)
{
    if (index < 0 || index > 3 || field != authum_entity_float_health)
    {
        return authum_bool_false;
    }
    *out = g_ent_health[index];
    return authum_bool_true;
}

authum_bool_t
test_ent_get_i32(authum_i32_t index, authum_i32_t field, authum_i32_t *out)
{
    if (index < 0 || index > 3)
    {
        return authum_bool_false;
    }
    if (field == authum_entity_i32_dead)
    {
        *out = g_ent_dead[index];
        return authum_bool_true;
    }
    if (field == authum_entity_i32_flags)
    {
        *out = g_ent_flags[index];
        return authum_bool_true;
    }
    return authum_bool_false;
}

void
test_engine_entities(void)
{
    authum_engine_t engine;
    authum_float_t health = -1.0f;
    authum_float_t vector[3] = {1.0f, 2.0f, 3.0f};
    authum_i32_t flags = -1;
    authum_char_t classname[16];

    memset(&engine, 0, sizeof engine);
    engine.size = sizeof engine;
    engine.client_is_connected = test_ent_connected;
    engine.entity_get_float = test_ent_get_float;
    engine.entity_get_i32 = test_ent_get_i32;

    CHECK(authum_engine_client_is_alive(&engine, 1));
    CHECK(!authum_engine_client_is_alive(&engine, 2)); /* dead */
    CHECK(authum_engine_client_is_alive(&engine, 3));
    CHECK(!authum_engine_client_is_alive(&engine, 4)); /* nobody there */
    CHECK(!authum_engine_client_is_bot(&engine, 1));
    CHECK(authum_engine_client_is_bot(&engine, 3));

    CHECK(authum_engine_entity_get_float(&engine, 3, authum_entity_float_health, &health));
    CHECK(health == 35.0f);
    CHECK(!authum_engine_entity_get_float(&engine, 3, authum_entity_float_armor, &health));
    CHECK(health == 0.0f); /* a refused read leaves 0, not the old value */
    CHECK(authum_engine_entity_get_i32(&engine, 3, authum_entity_i32_flags, &flags));
    CHECK((flags & authum_entity_flag_fake_client) != 0);

    /* Functions this fake engine does not have: false, and outputs zeroed. */
    CHECK(!authum_engine_entity_get_vector(&engine, 1, authum_entity_vector_origin, vector));
    CHECK(vector[0] == 0.0f && vector[1] == 0.0f && vector[2] == 0.0f);
    CHECK(!authum_engine_entity_set_float(&engine, 1, authum_entity_float_health, 1.0f));
    CHECK(!authum_engine_entity_get_classname(&engine, 1, classname, sizeof classname));
    CHECK_STR(classname, "");
    CHECK(authum_engine_entity_find_by_classname(&engine, 0, "player") == 0);
    CHECK(authum_engine_entity_max(authum_null) == 0);
    CHECK(!authum_engine_client_is_alive(authum_null, 1));
}

void
test_event_message(void)
{
    authum_event_message_t ev;

    memset(&ev, 0, sizeof ev);
    ev.size = sizeof ev;
    ev.argc = 3;
    ev.args[0].kind = authum_message_arg_byte;
    ev.args[0].value = 2;
    ev.args[1].kind = authum_message_arg_string;
    ev.args[1].text = 0u;
    memcpy(ev.text, "ak47", 5);
    ev.text_used = 5u;
    ev.args[2].kind = authum_message_arg_coord;
    ev.args[2].real = 1.5f;

    CHECK(authum_event_message_get_i32(&ev, 0) == 2);
    CHECK_STR(authum_event_message_get_string(&ev, 1), "ak47");
    CHECK(authum_event_message_get_float(&ev, 2) == 1.5f);
    CHECK_STR(authum_event_message_get_string(&ev, 0), ""); /* not a string */
    CHECK(authum_event_message_get_i32(&ev, 3) == 0);       /* out of range */

    CHECK(authum_event_message_set_i32(&ev, 0, 7) && authum_event_message_get_i32(&ev, 0) == 7);
    CHECK(!authum_event_message_set_i32(&ev, 1, 7)); /* a string stays a string */
    CHECK(authum_event_message_set_string(&ev, 1, "knife"));
    CHECK_STR(authum_event_message_get_string(&ev, 1), "knife");
    CHECK(!authum_event_message_set_string(&ev, 0, "x"));
    CHECK(authum_event_message_set_float(&ev, 2, -3.0f));
    CHECK(authum_event_message_get_float(&ev, 2) == -3.0f);

    ev.text_used = authum_event_message_text_max - 2u; /* no room left */
    CHECK(!authum_event_message_set_string(&ev, 1, "longer"));
    CHECK_STR(authum_event_message_get_string(&ev, 1), "knife");
}

/* --- configs/ and data/ with the old modules/<module>/ folder as a fallback --- */

#define TEST_ENV_ROOT "authum_env_test_root"

void
test_env_touch(const char *path, const char *text)
{
    FILE *file = fopen(path, "w");

    if (file != NULL)
    {
        fputs(text, file);
        fclose(file);
    }
}

authum_bool_t
test_env_exists(const char *path)
{
    FILE *file = fopen(path, "rb");

    if (file == NULL)
    {
        return authum_bool_false;
    }
    fclose(file);
    return authum_bool_true;
}

void
test_env_paths(void)
{
    authum_path_t root, modules, legacy, configs, data, module_config, module_data, out;
    authum_env_t env;

    authum_path_set_cstr(&root, TEST_ENV_ROOT);
    authum_path_join(&modules, &root, "modules");
    authum_path_join(&legacy, &modules, "foo");
    authum_path_join(&configs, &root, "configs");
    authum_path_join(&module_config, &configs, "foo");
    authum_path_join(&data, &root, "data");
    authum_path_join(&module_data, &data, "foo");
    CHECK(authum_path_ensure_dir(&root) && authum_path_ensure_dir(&modules) &&
          authum_path_ensure_dir(&legacy) && authum_path_ensure_dir(&configs) &&
          authum_path_ensure_dir(&module_config));
    remove(TEST_ENV_ROOT "/configs/foo.cfg");
    remove(TEST_ENV_ROOT "/configs/foo/extra.ini");
    remove(TEST_ENV_ROOT "/data/foo/foo.db");
    test_env_touch(TEST_ENV_ROOT "/modules/foo/foo.cfg", "old\n");
    test_env_touch(TEST_ENV_ROOT "/modules/foo/foo.db", "db\n");

    memset(&env, 0, sizeof env);
    env.size = sizeof env;
    env.modules = &modules;
    env.module = &legacy; /* an old modules/foo/foo.so */
    env.configs = &configs;
    env.module_config = &module_config;
    env.data = &data;
    env.module_data = &module_data;

    /* Only in the old folder: found there, flagged. */
    CHECK(authum_env_config_path(&env, "foo.cfg", &out) == authum_env_found_legacy);
    CHECK_STR(authum_path_as_cstr(&out), TEST_ENV_ROOT "/modules/foo/foo.cfg");
    /* In configs/ it wins. */
    test_env_touch(TEST_ENV_ROOT "/configs/foo.cfg", "new\n");
    CHECK(authum_env_config_path(&env, "foo.cfg", &out) == authum_env_found_here);
    CHECK_STR(authum_path_as_cstr(&out), TEST_ENV_ROOT "/configs/foo.cfg");
    /* A module's other files: configs/<module>/. */
    test_env_touch(TEST_ENV_ROOT "/configs/foo/extra.ini", "x\n");
    CHECK(authum_env_config_path(&env, "extra.ini", &out) == authum_env_found_here);
    CHECK_STR(authum_path_as_cstr(&out), TEST_ENV_ROOT "/configs/foo/extra.ini");
    /* Nowhere: where it belongs. */
    CHECK(authum_env_config_path(&env, "missing.cfg", &out) == authum_env_found_none);
    CHECK_STR(authum_path_as_cstr(&out), TEST_ENV_ROOT "/configs/missing.cfg");

    /* Data: moved out of the old folder once, then found in place. */
    CHECK(authum_env_data_path(&env, "foo.db", &out) == authum_env_found_legacy);
    CHECK_STR(authum_path_as_cstr(&out), TEST_ENV_ROOT "/data/foo/foo.db");
    CHECK(test_env_exists(TEST_ENV_ROOT "/data/foo/foo.db"));
    CHECK(!test_env_exists(TEST_ENV_ROOT "/modules/foo/foo.db"));
    CHECK(authum_env_data_path(&env, "foo.db", &out) == authum_env_found_here);
    CHECK(authum_env_data_path(&env, "new.db", &out) == authum_env_found_none);
    CHECK_STR(authum_path_as_cstr(&out), TEST_ENV_ROOT "/data/foo/new.db");

    /* A module right in modules/ has no old folder: modules/ is not searched. */
    test_env_touch(TEST_ENV_ROOT "/modules/bar.cfg", "shared dir\n");
    env.module = &modules;
    CHECK(authum_env_config_path(&env, "bar.cfg", &out) == authum_env_found_none);

    /* An older host's env (no size): no paths, nothing found, no crash. */
    env.size = 0u;
    CHECK(authum_env_configs(&env) == authum_null);
    CHECK(authum_env_config_path(&env, "foo.cfg", &out) == authum_env_found_none);
    CHECK(authum_env_config_path(authum_null, "foo.cfg", &out) == authum_env_found_none);

    remove(TEST_ENV_ROOT "/modules/foo/foo.cfg");
    remove(TEST_ENV_ROOT "/modules/bar.cfg");
}

/* A host built before a function existed hands a shorter table. */
void
test_abi_size(void)
{
    authum_engine_t engine;
    authum_char_t buf[16];

    memset(&engine, 0, sizeof engine);
    engine.server_command = test_engine_record_command;
    engine.query_client_cvar = test_engine_record_cvar;

    engine.size = 0u; /* no size: nothing is called */
    g_last_command[0] = '\0';
    authum_engine_server_command(&engine, "a\n");
    CHECK_STR(g_last_command, "");

    /* Ends right after server_command: that works, later fields do not. */
    engine.size = (authum_usize_t)((const char *)&engine.server_command - (const char *)&engine) +
                  sizeof engine.server_command;
    CHECK(AUTHUM_ABI_FITS(&engine, server_command));
    CHECK(!AUTHUM_ABI_FITS(&engine, max_clients));
    authum_engine_server_command(&engine, "b\n");
    CHECK_STR(g_last_command, "b\n");
    CHECK(!authum_engine_query_cvar(&engine, 2, "rate", 1));
    CHECK(authum_engine_max_clients(&engine) == 0);
    memcpy(buf, "junk", 5);
    CHECK(!authum_engine_client_get_name(&engine, 1, buf, sizeof buf));
    CHECK_STR(buf, ""); /* a refused getter still leaves a valid string */

    engine.size = sizeof engine;
    CHECK(AUTHUM_ABI_HAS(&engine, query_client_cvar));
    CHECK(!AUTHUM_ABI_HAS(&engine, max_clients)); /* in the table, but not set */
    CHECK(authum_engine_query_cvar(&engine, 2, "rate", 1));
}

void
test_host_copy(void)
{
    authum_host_t src;
    authum_host_t dst;
    authum_engine_t engine;

    memset(&engine, 0, sizeof engine);
    engine.size = sizeof engine;
    memset(&src, 0, sizeof src);
    src.size = sizeof src;
    src.engine = &engine;
    src.commands = (const authum_commands_t *)&engine; /* any non-null marker */

    authum_host_copy(&dst, &src);
    CHECK(dst.size == sizeof dst);
    CHECK(authum_host_engine(&dst) == &engine);
    CHECK(authum_host_commands(&dst) == src.commands);

    /* An older host: its table stops before `commands`. */
    src.size = (authum_usize_t)((const char *)&src.commands - (const char *)&src);
    memset(&dst, 0xff, sizeof dst);
    authum_host_copy(&dst, &src);
    CHECK(dst.size == src.size);
    CHECK(authum_host_engine(&dst) == &engine);
    CHECK(authum_host_commands(&dst) == authum_null);
    CHECK(dst.commands == authum_null); /* zeroed, not garbage */

    authum_host_clear(&dst);
    CHECK(authum_host_engine(&dst) == authum_null);
    CHECK(dst.size == src.size);

    authum_host_copy(&dst, authum_null);
    CHECK(dst.size == 0u);
}

void
test_abi_version(void)
{
    const authum_version_t v1_0 = AUTHUM_VERSION(1, 0, 0);
    const authum_version_t v1_3 = AUTHUM_VERSION(1, 3, 2);
    const authum_version_t v2_0 = AUTHUM_VERSION(2, 0, 0);
    const authum_version_t v0_3 = AUTHUM_VERSION(0, 3, 0);
    const authum_version_t v0_4 = AUTHUM_VERSION(0, 4, 0);
    const authum_version_t v0_4_1 = AUTHUM_VERSION(0, 4, 1);

    CHECK(authum_version_is_abi_compatible(&v1_0, &v1_3)); /* old module, new host */
    CHECK(authum_version_is_abi_compatible(&v1_3, &v1_0)); /* new module, old host */
    CHECK(!authum_version_is_abi_compatible(&v1_0, &v2_0));
    CHECK(!authum_version_is_abi_compatible(&v0_4, &v1_0));
    CHECK(!authum_version_is_abi_compatible(&v0_3, &v0_4));
    CHECK(authum_version_is_abi_compatible(&v0_4_1, &v0_4));
    CHECK(!authum_version_is_abi_compatible(authum_null, &v1_0));
}

void
test_hex(authum_byte_t *out, const char *hex)
{
    unsigned int byte;

    while (hex[0] != '\0' && hex[1] != '\0')
    {
        sscanf(hex, "%2x", &byte);
        *out++ = (authum_byte_t)byte;
        hex += 2;
    }
}

/* RFC 8032 section 7.1, TEST 1 (empty message) and TEST 2 (one byte 0x72). */
void
test_ed25519(void)
{
    authum_byte_t seed[AUTHUM_CRYPTO_ED25519_SEED_SIZE];
    authum_byte_t secret_key[AUTHUM_CRYPTO_ED25519_SECRET_KEY_SIZE];
    authum_byte_t public_key[AUTHUM_CRYPTO_ED25519_PUBLIC_KEY_SIZE];
    authum_byte_t expected_key[AUTHUM_CRYPTO_ED25519_PUBLIC_KEY_SIZE];
    authum_byte_t signature[AUTHUM_CRYPTO_ED25519_SIGNATURE_SIZE];
    authum_byte_t expected_sig[AUTHUM_CRYPTO_ED25519_SIGNATURE_SIZE];
    authum_byte_t message[1] = {0x72};
    authum_byte_t zeros[AUTHUM_CRYPTO_ED25519_SEED_SIZE];

    test_hex(seed, "9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60");
    test_hex(expected_key, "d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a");
    test_hex(expected_sig, "e5564300c360ac729086e2cc806e828a84877f1eb8e5d974d873e06522490155"
                           "5fb8821590a33bacc61e39701cf9b46bd25bf5f0595bbe24655141438e7a100b");
    authum_crypto_ed25519_key_pair(secret_key, public_key, seed);
    memset(zeros, 0, sizeof zeros);
    CHECK(memcmp(seed, zeros, sizeof zeros) == 0);
    CHECK(memcmp(public_key, expected_key, sizeof expected_key) == 0);
    authum_crypto_ed25519_sign(signature, secret_key, authum_null, 0u);
    CHECK(memcmp(signature, expected_sig, sizeof expected_sig) == 0);
    CHECK(authum_crypto_ed25519_check(signature, public_key, authum_null, 0u));

    test_hex(seed, "4ccd089b28ff96da9db6c346ec114e0f5b8a319f35aba624da8cf6ed4fb8a6fb");
    test_hex(expected_key, "3d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c");
    test_hex(expected_sig, "92a009a9f0d4cab8720e820b5f642540a2b27b5416503f8fb3762223ebdb69da"
                           "085ac1e43e15996e458f3613d0f11d8c387b2eaeb4302aeeb00d291612bb0c00");
    authum_crypto_ed25519_key_pair(secret_key, public_key, seed);
    CHECK(memcmp(public_key, expected_key, sizeof expected_key) == 0);
    authum_crypto_ed25519_sign(signature, secret_key, message, sizeof message);
    CHECK(memcmp(signature, expected_sig, sizeof expected_sig) == 0);
    CHECK(authum_crypto_ed25519_check(signature, public_key, message, sizeof message));

    /* Any flipped bit in message, signature or key must fail. */
    message[0] ^= 1u;
    CHECK(!authum_crypto_ed25519_check(signature, public_key, message, sizeof message));
    message[0] ^= 1u;
    signature[0] ^= 1u;
    CHECK(!authum_crypto_ed25519_check(signature, public_key, message, sizeof message));
    signature[0] ^= 1u;
    public_key[0] ^= 1u;
    CHECK(!authum_crypto_ed25519_check(signature, public_key, message, sizeof message));
    public_key[0] ^= 1u;
    CHECK(!authum_crypto_ed25519_check(authum_null, public_key, message, sizeof message));

    authum_crypto_wipe(secret_key, sizeof secret_key);
    CHECK(memcmp(secret_key, zeros, sizeof zeros) == 0);
}

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

void
test_detour(void)
{
    int (*volatile fn)(int) = test_detour_target;
    authum_detour_t other;

    CHECK(fn(1) == 2);
#if defined(__i386__)
    CHECK(authum_detour_attach(&g_test_detour, (authum_ptr)fn, (authum_ptr)&test_detour_hook));
    CHECK(!authum_detour_attach(&g_test_detour, (authum_ptr)fn, (authum_ptr)&test_detour_hook));
    CHECK(fn(1) == 200);
    CHECK(fn(4) == 500);
    authum_detour_suspend(&g_test_detour);
    CHECK(fn(1) == 2);
    authum_detour_resume(&g_test_detour);
    CHECK(fn(1) == 200);
    authum_detour_detach(&g_test_detour);
    CHECK(fn(1) == 2);
    authum_detour_detach(&g_test_detour); /* second detach is a no-op */
    CHECK(fn(1) == 2);
#endif
    memset(&other, 0, sizeof other);
    CHECK(!authum_detour_attach(&other, authum_null, (authum_ptr)&test_detour_hook));
    CHECK(!authum_detour_attach(authum_null, (authum_ptr)fn, (authum_ptr)&test_detour_hook));
}

#if defined(_WIN32)
#    define TEST_SYMBOL_LIBRARY "kernel32.dll"
#    define TEST_SYMBOL_NAME "lstrlenA"
#else
#    define TEST_SYMBOL_LIBRARY "libc.so.6"
#    define TEST_SYMBOL_NAME "strlen"
#endif

void
test_symbol(void)
{
    authum_bool_t via_dynsym = authum_bool_false;

    CHECK(authum_symbol_find(TEST_SYMBOL_LIBRARY, TEST_SYMBOL_NAME, &via_dynsym) != authum_null);
    CHECK(via_dynsym);
    CHECK(authum_symbol_find(TEST_SYMBOL_LIBRARY, "authum_no_such_symbol", &via_dynsym) ==
          authum_null);
    CHECK(!via_dynsym);
    CHECK(authum_symbol_find("authum_not_loaded.so", "strlen", authum_null) == authum_null);
    CHECK(authum_symbol_find_engine("SVC_Info", authum_null) == authum_null);
    CHECK(authum_symbol_find(authum_null, "strlen", authum_null) == authum_null);
}

void
test_net(void)
{
    authum_char_t text[AUTHUM_NET_IPV4_TEXT_SIZE];
    authum_u32_t ips[8];
    authum_usize_t n;
    authum_usize_t i;

    CHECK(authum_net_ipv4_is_external(0x25e6d2dau));  /* 37.230.210.218 */
    CHECK(authum_net_ipv4_is_external(0x08080808u));  /* 8.8.8.8 */
    CHECK(authum_net_ipv4_is_external(0xac200001u));  /* 172.32.0.1, just past 172.16/12 */
    CHECK(!authum_net_ipv4_is_external(0x7f000001u)); /* 127.0.0.1 */
    CHECK(!authum_net_ipv4_is_external(0x0a000001u)); /* 10.0.0.1 */
    CHECK(!authum_net_ipv4_is_external(0xac1f0001u)); /* 172.31.0.1 */
    CHECK(!authum_net_ipv4_is_external(0xc0a80101u)); /* 192.168.1.1 */
    CHECK(!authum_net_ipv4_is_external(0x64400001u)); /* 100.64.0.1 CGNAT */
    CHECK(!authum_net_ipv4_is_external(0xa9fe0101u)); /* 169.254.1.1 */
    CHECK(!authum_net_ipv4_is_external(0xe0000001u)); /* 224.0.0.1 */
    CHECK(!authum_net_ipv4_is_external(0x00000000u));
    CHECK(authum_net_ipv4_is_loopback(0x7f000001u));
    CHECK(!authum_net_ipv4_is_loopback(0x0a000001u));

    CHECK(authum_net_ipv4_format(0x25e6d2dau, text, sizeof text));
    CHECK_STR(text, "37.230.210.218");
    CHECK(authum_net_ipv4_format(0xffffffffu, text, sizeof text));
    CHECK_STR(text, "255.255.255.255");
    CHECK(!authum_net_ipv4_format(0x25e6d2dau, text, 8u));

    /* Real interfaces: whatever this machine has, the lists must be clean. */
    n = authum_net_ipv4_external(ips, 8u);
    for (i = 0u; i < n && i < 8u; i++)
    {
        CHECK(authum_net_ipv4_is_external(ips[i]));
    }
    n = authum_net_ipv4_internal(ips, 8u);
    for (i = 0u; i < n && i < 8u; i++)
    {
        CHECK(!authum_net_ipv4_is_external(ips[i]) && !authum_net_ipv4_is_loopback(ips[i]));
    }
    CHECK(authum_net_ipv4_internal(authum_null, 0u) == n); /* count without a buffer */
}

void
test_json(void)
{
    const char *reply =
        "{\"ip\":\"8.8.8.8\",\"countryCode\":\"US\",\"asn\":15169,"
        "\"isp\":\"Caf\\u00e9 \\\"Net\\\"\",\"note\":\"asn: 1\",\"big\":3000000000}";
    authum_char_t text[32];
    authum_char_t small[4];
    authum_i32_t n = 0;

    CHECK(authum_json_object_get_cstr(reply, "countryCode", text, sizeof text));
    CHECK_STR(text, "US");
    /* Escapes are decoded, é to UTF-8 included. */
    CHECK(authum_json_object_get_cstr(reply, "isp", text, sizeof text));
    CHECK_STR(text, "Caf\xC3\xA9 \"Net\"");
    /* Truncated to the buffer. */
    CHECK(authum_json_object_get_cstr(reply, "ip", small, sizeof small));
    CHECK_STR(small, "8.8");
    CHECK(!authum_json_object_get_cstr(reply, "asn", text, sizeof text)); /* not a string */
    CHECK(!authum_json_object_get_cstr(reply, "missing", text, sizeof text));
    CHECK_STR(text, "");

    CHECK(authum_json_object_get_i32(reply, "asn", &n) && n == 15169);
    CHECK(!authum_json_object_get_i32(reply, "big", &n)); /* past i32 */
    CHECK(!authum_json_object_get_i32(reply, "ip", &n));
    /* "asn: 1" inside a string value is text, not a field. */
    CHECK(authum_json_object_get_i32_or("{\"note\":\"asn: 1\"}", "asn", -1) == -1);
    CHECK(authum_json_object_get_i32_or("{\"asn\": 7}", "asn", -1) == 7);
    CHECK(authum_json_object_get_i32_or("{\"asn\": 7", "asn", -1) == -1); /* broken JSON */
    CHECK(authum_json_object_get_i32_or("[1,2]", "asn", -1) == -1);
    CHECK(authum_json_object_get_i32_or(authum_null, "asn", -1) == -1);
}

void
test_sql(void)
{
    authum_sql_t sql;
    authum_sql_stmt_t stmt;
    int rows = 0;

    authum_sql_init(&sql);
    CHECK(!authum_sql_is_open(&sql));
    CHECK(!authum_sql_exec(&sql, "SELECT 1"));

    CHECK(authum_sql_open_sqlite(&sql, ":memory:"));
    CHECK(authum_sql_is_open(&sql));
    CHECK(authum_sql_exec(&sql, "CREATE TABLE bans (id INTEGER PRIMARY KEY, steam INTEGER, "
                                "name TEXT, reason TEXT);"
                                "CREATE INDEX bans_steam ON bans(steam);"));

    CHECK(
        authum_sql_prepare(&sql, &stmt, "INSERT INTO bans (steam, name, reason) VALUES (?, ?, ?)"));
    CHECK(authum_sql_stmt_bind_i64(&stmt, 1, 76561198000000001LL));
    CHECK(authum_sql_stmt_bind_cstr(&stmt, 2, "O'Brien"));
    CHECK(authum_sql_stmt_bind_cstr(&stmt, 3, authum_null)); /* null text binds "" */
    CHECK(!authum_sql_stmt_bind_i32(&stmt, 0, 1));           /* parameters count from 1 */
    CHECK(authum_sql_stmt_step_done(&stmt));
    authum_sql_stmt_finalize(&stmt);
    CHECK(authum_sql_last_insert_id(&sql) == 1);

    CHECK(authum_sql_prepare(&sql, &stmt, "SELECT steam, name, reason FROM bans WHERE steam = ?"));
    CHECK(authum_sql_stmt_bind_i64(&stmt, 1, 76561198000000001LL));
    while (authum_sql_stmt_step_row(&stmt))
    {
        const authum_char_t *name = authum_sql_stmt_column_cstr(&stmt, 1);

        ++rows;
        CHECK(authum_sql_stmt_column_i64(&stmt, 0) == 76561198000000001LL);
        CHECK_STR(name, "O'Brien");
        CHECK_STR(authum_sql_stmt_column_cstr(&stmt, 2), "");
        CHECK_STR(authum_sql_stmt_column_cstr(&stmt, 7), ""); /* no such column */
        /* An earlier column's text is still there after reading another. */
        CHECK_STR(name, "O'Brien");
    }
    CHECK(rows == 1);
    authum_sql_stmt_finalize(&stmt);
    authum_sql_stmt_finalize(&stmt); /* a second finalize is a no-op */

    CHECK(!authum_sql_exec(&sql, "SELECT * FROM nowhere"));
    CHECK(strstr(authum_sql_errmsg(&sql), "no such table") != authum_null);
    CHECK(!authum_sql_prepare(&sql, &stmt, "SELEKT"));
    CHECK(authum_sql_errmsg(&sql)[0] != '\0');
    CHECK(authum_sql_exec(&sql, "SELECT 1"));
    CHECK_STR(authum_sql_errmsg(&sql), ""); /* a success clears the message */

    authum_sql_close(&sql);
    CHECK(!authum_sql_is_open(&sql));
    authum_sql_close(&sql);

    /* MySQL: with or without a client library here, nothing listens on port 1. */
    CHECK(!authum_sql_open_mysql(&sql, "127.0.0.1", "u", "p", "d", 1u));
    CHECK(!authum_sql_is_open(&sql));
    CHECK(authum_sql_errmsg(&sql)[0] != '\0');
}

int
main(void)
{
    test_json();
    test_sql();
    test_net();
    test_cstr();
    test_path();
    test_cfg();
    test_engine_kick();
    test_engine_query_cvar();
    test_engine_messages();
    test_event_message();
    test_engine_entities();
    test_env_paths();
    test_abi_size();
    test_host_copy();
    test_abi_version();
    test_ed25519();
    test_detour();
    test_symbol();
    return authum_check_report("authum_client");
}

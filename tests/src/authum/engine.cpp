#include <authum/engine.h>
#include <authum/cstr.h>
#include <authum/unused.h>
#include <gtest/gtest.h>
#include <stdio.h>
#include <string.h>

authum_char_t g_last_command[256];

void
test_engine_record_command(const authum_char_t *cmd)
{
    authum_cstr_copy(g_last_command, sizeof g_last_command, cmd);
}

TEST(authum_engine, sanitizes_kick_reason)
{
    authum_engine_t engine;

    memset(&engine, 0, sizeof engine);
    engine.server_command = test_engine_record_command;
    authum_engine_kick(&engine, 7, "bad \"guy\"; quit\n");
    EXPECT_STREQ(g_last_command, "kick #7 \"bad  guy   quit \"\n");
    authum_engine_kick(&engine, 3, authum_null);
    EXPECT_STREQ(g_last_command, "kick #3\n");
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

TEST(authum_engine, queries_cvar_with_request_identity)
{
    authum_engine_t engine;

    memset(&engine, 0, sizeof engine);
    EXPECT_FALSE(authum_engine_query_cvar(&engine, 2, "rate", 1));
    engine.query_client_cvar = test_engine_record_cvar;
    EXPECT_TRUE(authum_engine_query_cvar(&engine, 2, "cl_updaterate", 42));
    EXPECT_EQ(g_last_cvar_index, 2);
    EXPECT_EQ(g_last_cvar_request, 42);
    EXPECT_STREQ(g_last_cvar, "cl_updaterate");
    EXPECT_FALSE(authum_engine_query_cvar(&engine, 5, "rate", 1));
    EXPECT_FALSE(authum_engine_query_cvar(&engine, 2, "", 1));
    EXPECT_FALSE(authum_engine_query_cvar(&engine, 2, authum_null, 1));
    EXPECT_FALSE(authum_engine_query_cvar(authum_null, 2, "rate", 1));
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
             text != authum_null ? text : "-");
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
                                                          : authum_null;

    if (value == authum_null)
    {
        return authum_bool_false;
    }
    authum_cstr_copy(buf, buf_size, value);
    return authum_bool_true;
}

TEST(authum_engine, dispatches_user_messages_and_reads_cvars)
{
    authum_engine_t engine;

    memset(&engine, 0, sizeof engine);
    engine.message_id = test_msg_id;
    engine.message_begin = test_msg_begin;
    engine.message_write = test_msg_write;
    engine.message_end = test_msg_end;
    engine.cvar_get = test_cvar_get;

    g_msg_log[0] = '\0';
    EXPECT_TRUE(authum_engine_client_chat(&engine, 3, 3, "hi"));
    EXPECT_STREQ(g_msg_log, "begin 1 76 3;w0 3 0.00 -;w6 0 0.00 hi;end;");

    g_msg_log[0] = '\0';
    EXPECT_TRUE(authum_engine_client_chat(&engine, 0, 0, "all")); /* everyone: dest all */
    EXPECT_STREQ(g_msg_log, "begin 2 76 0;w0 0 0.00 -;w6 0 0.00 all;end;");

    /* A refused begin: the writes and the end after it go nowhere. */
    g_msg_log[0] = '\0';
    EXPECT_FALSE(authum_engine_client_chat(&engine, 9, 0, "x"));
    EXPECT_FALSE(authum_engine_message_begin(&engine, authum_message_dest_one, 76, 9));
    authum_engine_message_write_coord(&engine, 1.5f);
    authum_engine_message_end(&engine);
    EXPECT_STREQ(g_msg_log, "");

    g_msg_log[0] = '\0';
    EXPECT_TRUE(authum_engine_message_begin(&engine, authum_message_dest_all, 76, 0));
    authum_engine_message_write_coord(&engine, 1.5f);
    authum_engine_message_write_string(&engine, authum_null);
    authum_engine_message_end(&engine);
    EXPECT_STREQ(g_msg_log, "begin 2 76 0;w5 0 1.50 -;w6 0 0.00 ;end;");

    EXPECT_EQ(authum_engine_message_get_id(&engine, "DeathMsg"), 0);
    EXPECT_FALSE(authum_engine_message_begin(&engine, authum_message_dest_all, 0, 0)); /* no id */
    engine.message_id = 0;
    EXPECT_FALSE(authum_engine_client_chat(&engine, 1, 1, "no SayText")); /* host without it */

    EXPECT_EQ(authum_engine_cvar_get_i32(&engine, "a", 5), 12);
    EXPECT_EQ(authum_engine_cvar_get_i32(&engine, "c", 5), 5); /* not a number */
    EXPECT_EQ(authum_engine_cvar_get_i32(&engine, "missing", 5), 5);
    EXPECT_EQ(authum_engine_cvar_get_float(&engine, "b", 1.0f), 0.25f);
    EXPECT_EQ(authum_engine_cvar_get_float(&engine, "c", 1.0f), 1.0f);
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

TEST(authum_engine, reads_and_updates_entity_state)
{
    authum_engine_t engine;
    authum_float_t health = -1.0f;
    authum_float_t vector[3] = {1.0f, 2.0f, 3.0f};
    authum_i32_t flags = -1;
    authum_char_t classname[16];

    memset(&engine, 0, sizeof engine);
    engine.client_is_connected = test_ent_connected;
    engine.entity_get_float = test_ent_get_float;
    engine.entity_get_i32 = test_ent_get_i32;

    EXPECT_TRUE(authum_engine_client_is_alive(&engine, 1));
    EXPECT_FALSE(authum_engine_client_is_alive(&engine, 2)); /* dead */
    EXPECT_TRUE(authum_engine_client_is_alive(&engine, 3));
    EXPECT_FALSE(authum_engine_client_is_alive(&engine, 4)); /* nobody there */
    EXPECT_FALSE(authum_engine_client_is_bot(&engine, 1));
    EXPECT_TRUE(authum_engine_client_is_bot(&engine, 3));

    EXPECT_TRUE(
        authum_engine_entity_try_get_float(&engine, 3, authum_entity_float_health, &health));
    EXPECT_EQ(health, 35.0f);
    EXPECT_FALSE(
        authum_engine_entity_try_get_float(&engine, 3, authum_entity_float_armor, &health));
    EXPECT_EQ(health, 0.0f); /* a refused read leaves 0, not the old value */
    EXPECT_TRUE(authum_engine_entity_try_get_i32(&engine, 3, authum_entity_i32_flags, &flags));
    EXPECT_NE((flags & authum_entity_flag_fake_client), 0);

    /* Functions this fake engine does not have: false, and outputs zeroed. */
    EXPECT_FALSE(authum_engine_entity_copy_vector(&engine, 1, authum_entity_vector_origin, vector));
    EXPECT_TRUE(vector[0] == 0.0f && vector[1] == 0.0f && vector[2] == 0.0f);
    EXPECT_FALSE(authum_engine_entity_set_float(&engine, 1, authum_entity_float_health, 1.0f));
    EXPECT_FALSE(authum_engine_entity_copy_classname(&engine, 1, classname, sizeof classname));
    EXPECT_STREQ(classname, "");
    EXPECT_EQ(authum_engine_entity_find_by_classname(&engine, 0, "player"), 0);
    EXPECT_EQ(authum_engine_entity_get_max(authum_null), 0);
    EXPECT_FALSE(authum_engine_client_is_alive(authum_null, 1));
}

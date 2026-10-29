/**
 * @file commands.cpp
 * @brief Command registry: scoping, conflicts, args, usage, owner cleanup.
 */

#include <gtest/gtest.h>

#include <authum/host/commands.h>
#include <authum/null.h>
#include <authum/unused.h>

#include <string.h>

authum_char_t g_out[1024];
authum_i32_t g_last_number;

void
test_print(authum_ptr out, const authum_char_t *text)
{
    authum_unused(out);
    strncat(g_out, text, sizeof g_out - strlen(g_out) - 1u);
}

authum_bool_t
test_cmd_echo(const authum_command_t *cmd, authum_ptr user)
{
    authum_usize_t i;

    authum_unused(user);
    for (i = 0u; i < authum_command_get_argc(cmd); ++i)
    {
        authum_command_printf(cmd, "%s/%s:%s", authum_command_get_scope(cmd),
                              authum_command_get_name(cmd), authum_command_get_argv(cmd, i));
    }
    return authum_bool_true;
}

authum_bool_t
test_cmd_number(const authum_command_t *cmd, authum_ptr user)
{
    authum_unused(user);
    if (authum_command_get_argc(cmd) != 1u ||
        !authum_command_try_get_arg_i32(cmd, 0u, &g_last_number))
    {
        return authum_bool_false;
    }
    authum_command_printf(cmd, "n=%d", (int)g_last_number);
    return authum_bool_true;
}

authum_commands_t
test_commands_for(authum_ptr owner, const authum_char_t *scope)
{
    authum_commands_t c;

    c.ops = authum_commands_host_ops();
    c.owner = owner;
    c.scope = scope;
    return c;
}

authum_bool_t
test_cmd_self_remove(const authum_command_t *cmd, authum_ptr user)
{
    authum_commands_remove((const authum_commands_t *)user, authum_command_get_name(cmd));
    authum_command_print(cmd, "bye");
    return authum_bool_true;
}

void
test_reset_out(void)
{
    g_out[0] = '\0';
}

TEST(authum_commands, words)
{
    EXPECT_TRUE(authum_commands_host_is_word("qproto"));
    EXPECT_TRUE(authum_commands_host_is_word("auth.v2-x_1"));
    EXPECT_FALSE(authum_commands_host_is_word(""));
    EXPECT_FALSE(authum_commands_host_is_word("has space"));
    EXPECT_FALSE(authum_commands_host_is_word("semi;colon"));
    EXPECT_FALSE(authum_commands_host_is_word(authum_null));
    EXPECT_FALSE(authum_commands_host_is_word("0123456789012345678901234567890123"));
}

TEST(authum_commands, run_and_scope)
{
    int owner_a = 0;
    int owner_b = 0;
    authum_commands_t a = test_commands_for(&owner_a, "alpha");
    authum_commands_t b = test_commands_for(&owner_b, "beta");
    const authum_command_def_t defs[] = {
        {"echo", "<words…>", "print args", test_cmd_echo, authum_null},
        {"number", "<n>", "parse a number", test_cmd_number, authum_null},
    };
    const authum_command_def_t beta_echo = {"echo", authum_null, authum_null, test_cmd_echo,
                                            authum_null};
    const authum_char_t *args[] = {"x", "y"};
    const authum_char_t *bad[] = {"nope"};
    const authum_char_t *num[] = {"-7"};

    EXPECT_TRUE(authum_commands_add_all(&a, defs, 2u));
    /* Same def again from the same owner is fine; a different handler is a conflict. */
    EXPECT_TRUE(authum_commands_add(&a, &defs[0]));
    {
        authum_command_def_t clash = defs[0];
        clash.fn = test_cmd_number;
        EXPECT_FALSE(authum_commands_add(&a, &clash));
    }
    /* Same word in another module's scope does not clash. */
    EXPECT_TRUE(authum_commands_add(&b, &beta_echo));
    EXPECT_EQ(authum_commands_host_count(), 3u);
    EXPECT_TRUE(authum_commands_host_has_scope("ALPHA"));
    EXPECT_FALSE(authum_commands_host_has_scope("gamma"));

    test_reset_out();
    EXPECT_TRUE(authum_commands_host_run("alpha", "ECHO", 2u, args, test_print, authum_null));
    EXPECT_STREQ(g_out, "alpha/echo:x\nalpha/echo:y\n");

    test_reset_out();
    EXPECT_TRUE(authum_commands_host_run("alpha", "number", 1u, num, test_print, authum_null));
    EXPECT_EQ(g_last_number, -7);
    EXPECT_STREQ(g_out, "n=-7\n");

    test_reset_out();
    EXPECT_TRUE(authum_commands_host_run("alpha", "number", 1u, bad, test_print, authum_null));
    EXPECT_STREQ(g_out, "usage: authum alpha number <n>  - parse a number\n");

    test_reset_out();
    EXPECT_TRUE(
        authum_commands_host_run("alpha", "number", 0u, authum_null, test_print, authum_null));
    EXPECT_EQ(strstr(g_out, "usage:"), g_out);

    EXPECT_FALSE(
        authum_commands_host_run("alpha", "missing", 0u, authum_null, test_print, authum_null));
    EXPECT_FALSE(
        authum_commands_host_run("gamma", "echo", 0u, authum_null, test_print, authum_null));

    test_reset_out();
    EXPECT_EQ(authum_commands_host_print_help("beta", test_print, authum_null), 1u);
    EXPECT_STREQ(g_out, "  authum beta echo\n");
    EXPECT_EQ(authum_commands_host_print_help(authum_null, test_print, authum_null), 3u);

    /* Another owner cannot remove alpha's command. */
    {
        authum_commands_t impostor = test_commands_for(&owner_b, "alpha");
        EXPECT_FALSE(authum_commands_remove(&impostor, "number"));
    }
    EXPECT_FALSE(authum_commands_remove(&b, "number"));
    EXPECT_TRUE(authum_commands_remove(&a, "number"));
    EXPECT_EQ(authum_commands_host_count(), 2u);

    authum_commands_host_drop_owner(&owner_a);
    EXPECT_FALSE(authum_commands_host_has_scope("alpha"));
    EXPECT_TRUE(authum_commands_host_has_scope("beta"));
    authum_commands_host_clear();
    EXPECT_EQ(authum_commands_host_count(), 0u);
}

TEST(authum_commands, self_remove)
{
    int owner = 0;
    authum_commands_t c = test_commands_for(&owner, "self");
    authum_command_def_t def = {"once", authum_null, authum_null, test_cmd_self_remove, &c};

    EXPECT_TRUE(authum_commands_add(&c, &def));
    test_reset_out();
    EXPECT_TRUE(authum_commands_host_run("self", "once", 0u, authum_null, test_print, authum_null));
    EXPECT_STREQ(g_out, "bye\n");
    EXPECT_EQ(authum_commands_host_count(), 0u);
    EXPECT_FALSE(
        authum_commands_host_run("self", "once", 0u, authum_null, test_print, authum_null));
}

TEST(authum_commands, invalid)
{
    int owner = 0;
    authum_commands_t c = test_commands_for(&owner, "bad scope");
    authum_commands_t ok = test_commands_for(&owner, "ok");
    authum_command_def_t def = {"x", authum_null, authum_null, test_cmd_echo, authum_null};
    authum_command_def_t no_fn = {"y", authum_null, authum_null, 0, authum_null};
    authum_command_def_t bad_name = {"a b", authum_null, authum_null, test_cmd_echo, authum_null};

    EXPECT_FALSE(authum_commands_add(&c, &def));
    EXPECT_FALSE(authum_commands_add(&ok, &no_fn));
    EXPECT_FALSE(authum_commands_add(&ok, &bad_name));
    EXPECT_FALSE(authum_commands_add(authum_null, &def));
    EXPECT_FALSE(authum_commands_add(&ok, authum_null));
    EXPECT_EQ(authum_commands_host_count(), 0u);
}

TEST(authum_commands, command_accessors)
{
    authum_command_t cmd;
    authum_i32_t n = 0;
    const authum_char_t *args[] = {"12", "abc"};

    memset(&cmd, 0, sizeof cmd);
    cmd.argc = 2u;
    cmd.argv = args;
    cmd.print = test_print;
    EXPECT_STREQ(authum_command_get_argv(&cmd, 1u), "abc");
    EXPECT_STREQ(authum_command_get_argv(&cmd, 5u), "");
    EXPECT_STREQ(authum_command_get_scope(&cmd), "");
    EXPECT_TRUE(authum_command_try_get_arg_i32(&cmd, 0u, &n) && n == 12);
    EXPECT_FALSE(authum_command_try_get_arg_i32(&cmd, 1u, &n));
    EXPECT_FALSE(authum_command_try_get_arg_i32(&cmd, 2u, &n));
    EXPECT_EQ(authum_command_get_argc(authum_null), 0u);
    EXPECT_STREQ(authum_command_get_argv(authum_null, 0u), "");
    authum_command_print(authum_null, "ignored");
}

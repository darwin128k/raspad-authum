/**
 * @file commands.c
 * @brief Command registry: scoping, conflicts, args, usage, owner cleanup.
 */

#include <check.h>

#include <authum/char.h>
#include <authum/command.h>
#include <authum/commands.h>
#include <authum/commands/host.h>
#include <authum/null.h>

#include <string.h>

authum_char_t g_out[1024];
authum_i32_t g_last_number;

void
test_print(authum_ptr out, const authum_char_t *text)
{
    (void)out;
    strncat(g_out, text, sizeof g_out - strlen(g_out) - 1u);
}

authum_bool_t
test_cmd_echo(const authum_command_t *cmd, authum_ptr user)
{
    authum_usize_t i;

    (void)user;
    for (i = 0u; i < authum_command_argc(cmd); ++i) {
        authum_command_printf(cmd, "%s/%s:%s", authum_command_scope(cmd), authum_command_name(cmd),
                              authum_command_argv(cmd, i));
    }
    return authum_bool_true;
}

authum_bool_t
test_cmd_number(const authum_command_t *cmd, authum_ptr user)
{
    (void)user;
    if (authum_command_argc(cmd) != 1u || !authum_command_arg_i32(cmd, 0u, &g_last_number)) {
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
    authum_commands_remove((const authum_commands_t *)user, authum_command_name(cmd));
    authum_command_print(cmd, "bye");
    return authum_bool_true;
}

void
test_reset_out(void)
{
    g_out[0] = '\0';
}

void
test_words(void)
{
    CHECK(authum_commands_host_is_word("qproto"));
    CHECK(authum_commands_host_is_word("auth.v2-x_1"));
    CHECK(!authum_commands_host_is_word(""));
    CHECK(!authum_commands_host_is_word("has space"));
    CHECK(!authum_commands_host_is_word("semi;colon"));
    CHECK(!authum_commands_host_is_word(authum_null));
    CHECK(!authum_commands_host_is_word("0123456789012345678901234567890123"));
}

void
test_run_and_scope(void)
{
    int owner_a = 0;
    int owner_b = 0;
    authum_commands_t a = test_commands_for(&owner_a, "alpha");
    authum_commands_t b = test_commands_for(&owner_b, "beta");
    const authum_command_def_t defs[] = {
        { "echo", "<words…>", "print args", test_cmd_echo, authum_null },
        { "number", "<n>", "parse a number", test_cmd_number, authum_null },
    };
    const authum_command_def_t beta_echo = { "echo", authum_null, authum_null, test_cmd_echo,
                                             authum_null };
    const authum_char_t *args[] = { "x", "y" };
    const authum_char_t *bad[] = { "nope" };
    const authum_char_t *num[] = { "-7" };

    CHECK(authum_commands_add_all(&a, defs, 2u));
    /* Same def again from the same owner is fine; a different handler is a conflict. */
    CHECK(authum_commands_add(&a, &defs[0]));
    {
        authum_command_def_t clash = defs[0];
        clash.fn = test_cmd_number;
        CHECK(!authum_commands_add(&a, &clash));
    }
    /* Same word in another module's scope does not clash. */
    CHECK(authum_commands_add(&b, &beta_echo));
    CHECK(authum_commands_host_count() == 3u);
    CHECK(authum_commands_host_has_scope("ALPHA"));
    CHECK(!authum_commands_host_has_scope("gamma"));

    test_reset_out();
    CHECK(authum_commands_host_run("alpha", "ECHO", 2u, args, test_print, authum_null));
    CHECK_STR(g_out, "alpha/echo:x\nalpha/echo:y\n");

    test_reset_out();
    CHECK(authum_commands_host_run("alpha", "number", 1u, num, test_print, authum_null));
    CHECK(g_last_number == -7);
    CHECK_STR(g_out, "n=-7\n");

    test_reset_out();
    CHECK(authum_commands_host_run("alpha", "number", 1u, bad, test_print, authum_null));
    CHECK_STR(g_out, "usage: authum alpha number <n>  - parse a number\n");

    test_reset_out();
    CHECK(authum_commands_host_run("alpha", "number", 0u, authum_null, test_print, authum_null));
    CHECK(strstr(g_out, "usage:") == g_out);

    CHECK(!authum_commands_host_run("alpha", "missing", 0u, authum_null, test_print, authum_null));
    CHECK(!authum_commands_host_run("gamma", "echo", 0u, authum_null, test_print, authum_null));

    test_reset_out();
    CHECK(authum_commands_host_print_help("beta", test_print, authum_null) == 1u);
    CHECK_STR(g_out, "  authum beta echo\n");
    CHECK(authum_commands_host_print_help(authum_null, test_print, authum_null) == 3u);

    /* Another owner cannot remove alpha's command. */
    {
        authum_commands_t impostor = test_commands_for(&owner_b, "alpha");
        CHECK(!authum_commands_remove(&impostor, "number"));
    }
    CHECK(!authum_commands_remove(&b, "number"));
    CHECK(authum_commands_remove(&a, "number"));
    CHECK(authum_commands_host_count() == 2u);

    authum_commands_host_drop_owner(&owner_a);
    CHECK(!authum_commands_host_has_scope("alpha"));
    CHECK(authum_commands_host_has_scope("beta"));
    authum_commands_host_clear();
    CHECK(authum_commands_host_count() == 0u);
}

void
test_self_remove(void)
{
    int owner = 0;
    authum_commands_t c = test_commands_for(&owner, "self");
    authum_command_def_t def = { "once", authum_null, authum_null, test_cmd_self_remove, &c };

    CHECK(authum_commands_add(&c, &def));
    test_reset_out();
    CHECK(authum_commands_host_run("self", "once", 0u, authum_null, test_print, authum_null));
    CHECK_STR(g_out, "bye\n");
    CHECK(authum_commands_host_count() == 0u);
    CHECK(!authum_commands_host_run("self", "once", 0u, authum_null, test_print, authum_null));
}

void
test_invalid(void)
{
    int owner = 0;
    authum_commands_t c = test_commands_for(&owner, "bad scope");
    authum_commands_t ok = test_commands_for(&owner, "ok");
    authum_command_def_t def = { "x", authum_null, authum_null, test_cmd_echo, authum_null };
    authum_command_def_t no_fn = { "y", authum_null, authum_null, 0, authum_null };
    authum_command_def_t bad_name = { "a b", authum_null, authum_null, test_cmd_echo, authum_null };

    CHECK(!authum_commands_add(&c, &def));
    CHECK(!authum_commands_add(&ok, &no_fn));
    CHECK(!authum_commands_add(&ok, &bad_name));
    CHECK(!authum_commands_add(authum_null, &def));
    CHECK(!authum_commands_add(&ok, authum_null));
    CHECK(authum_commands_host_count() == 0u);
}

void
test_command_accessors(void)
{
    authum_command_t cmd;
    authum_i32_t n = 0;
    const authum_char_t *args[] = { "12", "abc" };

    memset(&cmd, 0, sizeof cmd);
    cmd.argc = 2u;
    cmd.argv = args;
    cmd.print = test_print;
    CHECK_STR(authum_command_argv(&cmd, 1u), "abc");
    CHECK_STR(authum_command_argv(&cmd, 5u), "");
    CHECK_STR(authum_command_scope(&cmd), "");
    CHECK(authum_command_arg_i32(&cmd, 0u, &n) && n == 12);
    CHECK(!authum_command_arg_i32(&cmd, 1u, &n));
    CHECK(!authum_command_arg_i32(&cmd, 2u, &n));
    CHECK(authum_command_argc(authum_null) == 0u);
    CHECK_STR(authum_command_argv(authum_null, 0u), "");
    authum_command_print(authum_null, "ignored");
}

int
main(void)
{
    test_words();
    test_run_and_scope();
    test_self_remove();
    test_invalid();
    test_command_accessors();
    return authum_check_report("authum_commands");
}

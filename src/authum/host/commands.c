/**
 * @file commands.c
 * @brief Module commands over the LH console ::lh_console_t.
 *
 * LH owns the registry: scopes, owners, case-insensitive lookup and removal while a
 * handler runs. Authum adds the module ABI over it, the word length limit and the
 * `authum <scope> <name>` help line. All calls run on the server thread.
 */

#include <authum/host/commands.h>
#include <authum/null.h>
#include <lh/char/map.h>
#include <lh/console.h>
#include <lh/util/return.h>

static lh_console_t authum_commands_console;
static authum_bool_t authum_commands_initialized;

static lh_console_t *
authum_commands_host_get_console(void)
{
    if (!authum_commands_initialized)
    {
        lh_console_init(lh_addr_of(authum_commands_console));
        authum_commands_initialized = authum_bool_true;
    }
    return lh_addr_of(authum_commands_console);
}

authum_bool_t
authum_commands_host_is_word(const authum_char_t *text)
{
    return lh_console_is_word(text) &&
                   lh_math_lt(lh_str_ptr_len(text),
                              lh_cast_static(authum_usize_t, authum_command_word_max))
               ? authum_bool_true
               : authum_bool_false;
}

/* A module's handler is the console handler itself: ::authum_command_t is laid out as
   ::lh_console_call_t (checked in src/authum/api/command.c), so the module ABI meets the
   console here, in one cast. */
authum_bool_t
authum_commands_host_add(authum_ptr owner, const authum_char_t *scope,
                         const authum_command_def_t *def)
{
    lh_console_command_t command;

    lh_return_if(authum_null_eq(def) || !authum_commands_host_is_word(scope) ||
                     !authum_commands_host_is_word(def->name),
                 authum_bool_false);
    lh_console_command_set_name(lh_addr_of(command), def->name);
    lh_console_command_set_usage(lh_addr_of(command), def->usage);
    lh_console_command_set_help(lh_addr_of(command), def->help);
    lh_console_command_set_fn(lh_addr_of(command), lh_cast_reinterpret(lh_console_cb, def->fn));
    lh_console_command_set_user(lh_addr_of(command), def->user);
    return lh_console_add(authum_commands_host_get_console(), owner, scope, lh_addr_of(command));
}

authum_bool_t
authum_commands_host_remove(authum_ptr owner, const authum_char_t *scope, const authum_char_t *name)
{
    return lh_console_remove(authum_commands_host_get_console(), owner, scope, name);
}

const authum_commands_ops_t authum_commands_host_ops_table = {
    .add = authum_commands_host_add,
    .remove = authum_commands_host_remove,
};

const authum_commands_ops_t *
authum_commands_host_ops(void)
{
    return lh_addr_of(authum_commands_host_ops_table);
}

static void
authum_commands_host_print_entry(const lh_console_entry_t *entry, authum_command_print_fn print,
                                 authum_ptr out, const authum_char_t *prefix)
{
    const authum_char_t *usage = lh_str_get_data(lh_console_entry_get_usage(entry));
    const authum_char_t *help = lh_str_get_data(lh_console_entry_get_help(entry));
    lh_console_call_t call;

    lh_console_call_init(lh_addr_of(call), authum_null, authum_null, 0u, authum_null, print, out);
    lh_console_call_printf(lh_addr_of(call), "%sauthum %s %s%s%s%s%s", prefix,
                           lh_str_get_data(lh_console_entry_get_scope(entry)),
                           lh_str_get_data(lh_console_entry_get_name(entry)),
                           lh_math_ne(usage[0], lh_char_map_nul) ? " " : "", usage,
                           lh_math_ne(help[0], lh_char_map_nul) ? "  - " : "", help);
}

authum_bool_t
authum_commands_host_run(const authum_char_t *scope, const authum_char_t *name, authum_usize_t argc,
                         const authum_char_t *const *argv, authum_command_print_fn print,
                         authum_ptr out)
{
    lh_console_t *console = authum_commands_host_get_console();
    const lh_console_entry_t *ran = authum_null;
    lh_console_result_t result;

    /* Hold one run open, so the command that ran stays readable for its usage line even
       when its handler removed it. */
    lh_console_set_depth(console, lh_math_add_one(lh_console_get_depth(console)));
    result = lh_console_run(console, scope, name, argc, argv, print, out, lh_addr_of(ran));
    if (lh_math_eq(result, lh_console_result_bad_usage))
    {
        authum_commands_host_print_entry(ran, print, out, "usage: ");
    }
    lh_console_set_depth(console, lh_math_sub_one(lh_console_get_depth(console)));
    lh_console_collect(console);
    return lh_math_ne(result, lh_console_result_not_found) ? authum_bool_true : authum_bool_false;
}

authum_bool_t
authum_commands_host_has_scope(const authum_char_t *scope)
{
    return lh_console_has_scope(authum_commands_host_get_console(), scope);
}

authum_usize_t
authum_commands_host_print_help(const authum_char_t *scope, authum_command_print_fn print,
                                authum_ptr out)
{
    const lh_console_t *console = authum_commands_host_get_console();
    authum_usize_t n = 0u;

    for (const lh_console_entry_t *entry = lh_console_get_first(console); !authum_null_eq(entry);
         entry = lh_console_get_next(console, entry))
    {
        if (authum_null_eq(scope) || lh_console_entry_is_named(entry, scope, authum_null))
        {
            authum_commands_host_print_entry(entry, print, out, "  ");
            n = lh_math_add_one(n);
        }
    }
    return n;
}

void
authum_commands_host_drop_owner(authum_ptr owner)
{
    lh_console_drop_owner(authum_commands_host_get_console(), owner);
}

authum_usize_t
authum_commands_host_count(void)
{
    return lh_console_get_count(authum_commands_host_get_console());
}

void
authum_commands_host_clear(void)
{
    lh_console_clear(authum_commands_host_get_console());
}

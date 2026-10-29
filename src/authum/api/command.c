/**
 * @file command.c
 * @brief ::authum_command_t over the LH console call ::lh_console_call_t.
 *
 * The host hands a module's handler the console call itself (src/authum/host/commands.c):
 * the two structures are laid out alike, checked here. A ::authum_null command reads
 * as empty and prints nothing.
 */

#include <authum/command.h>
#include <lh/assert/static.h>
#include <lh/bool.h>
#include <lh/console/call.h>
#include <lh/util/offset.h>
#include <lh/util/return.h>

#include <authum/cstr.h>

lh_assert_static(
    sizeof(authum_command_t) == sizeof(lh_console_call_t) &&
        lh_offset_of(authum_command_t, scope) == lh_offset_of(lh_console_call_t, scope) &&
        lh_offset_of(authum_command_t, name) == lh_offset_of(lh_console_call_t, name) &&
        lh_offset_of(authum_command_t, argc) == lh_offset_of(lh_console_call_t, argc) &&
        lh_offset_of(authum_command_t, argv) == lh_offset_of(lh_console_call_t, argv) &&
        lh_offset_of(authum_command_t, print) == lh_offset_of(lh_console_call_t, sink) &&
        lh_offset_of(authum_command_t, out) == lh_offset_of(lh_console_call_t, out),
    "authum_command_t must be laid out as lh_console_call_t");
lh_assert_static(sizeof(authum_usize_t) == sizeof(lh_usize_t) &&
                     sizeof(authum_bool_t) == sizeof(lh_bool_t),
                 "authum and lh sizes and booleans must match");

/* The console call @p cmd is. */
static const lh_console_call_t *
authum_command_get_call(const authum_command_t *cmd)
{
    return lh_ptr_rcast(const lh_console_call_t, cmd);
}

const authum_char_t *
authum_command_get_scope(const authum_command_t *cmd)
{
    lh_return_if(authum_null_eq(cmd), "");
    return authum_null_eq(lh_console_call_get_scope(authum_command_get_call(cmd)))
               ? ""
               : lh_console_call_get_scope(authum_command_get_call(cmd));
}

const authum_char_t *
authum_command_get_name(const authum_command_t *cmd)
{
    lh_return_if(authum_null_eq(cmd), "");
    return authum_null_eq(lh_console_call_get_name(authum_command_get_call(cmd)))
               ? ""
               : lh_console_call_get_name(authum_command_get_call(cmd));
}

authum_usize_t
authum_command_get_argc(const authum_command_t *cmd)
{
    lh_return_if(authum_null_eq(cmd) ||
                     authum_null_eq(lh_console_call_get_argv(authum_command_get_call(cmd))),
                 0u);
    return lh_console_call_get_argc(authum_command_get_call(cmd));
}

const authum_char_t *
authum_command_get_argv(const authum_command_t *cmd, authum_usize_t index)
{
    lh_return_if(lh_math_ge(index, authum_command_get_argc(cmd)), "");
    return lh_console_call_get_arg(authum_command_get_call(cmd), index);
}

authum_bool_t
authum_command_try_get_arg_i32(const authum_command_t *cmd, authum_usize_t index, authum_i32_t *out)
{
    lh_return_if(lh_math_ge(index, authum_command_get_argc(cmd)), authum_bool_false);
    return authum_cstr_parse_i32(authum_command_get_argv(cmd, index), out);
}

void
authum_command_print(const authum_command_t *cmd, const authum_char_t *line)
{
    lh_return_if(authum_null_eq(cmd));
    lh_console_call_print(authum_command_get_call(cmd), line);
}

void
authum_command_printf(const authum_command_t *cmd, const authum_char_t *fmt, ...)
{
    va_list args;

    lh_return_if(authum_null_eq(cmd));
    va_start(args, fmt);
    lh_console_call_printf_v(authum_command_get_call(cmd), fmt, args);
    va_end(args);
}

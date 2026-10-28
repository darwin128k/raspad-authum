#include <authum/command.h>
#include <authum/unused.h>
#include <lh/char/map.h>
#include <lh/memory/std.h>

#include <authum/cstr.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

const authum_char_t *
authum_command_scope(const authum_command_t *cmd)
{
    return (!authum_null_eq(cmd) && !authum_null_eq(cmd->scope)) ? cmd->scope : "";
}

const authum_char_t *
authum_command_name(const authum_command_t *cmd)
{
    return (!authum_null_eq(cmd) && !authum_null_eq(cmd->name)) ? cmd->name : "";
}

authum_usize_t
authum_command_argc(const authum_command_t *cmd)
{
    return (!authum_null_eq(cmd) && !authum_null_eq(cmd->argv)) ? cmd->argc : 0u;
}

const authum_char_t *
authum_command_argv(const authum_command_t *cmd, authum_usize_t index)
{
    const authum_char_t *arg;

    if (lh_math_ge(index, authum_command_argc(cmd)))
    {
        return "";
    }
    arg = cmd->argv[index];
    return authum_null_eq(arg) ? "" : arg;
}

authum_bool_t
authum_command_arg_i32(const authum_command_t *cmd, authum_usize_t index, authum_i32_t *out)
{
    if (lh_math_ge(index, authum_command_argc(cmd)))
    {
        return authum_bool_false;
    }
    return authum_cstr_parse_i32(authum_command_argv(cmd, index), out);
}

void
authum_command_print(const authum_command_t *cmd, const authum_char_t *line)
{
    authum_char_t buf[lh_math_add(authum_command_line_max, 2)];
    authum_usize_t len;

    if (authum_null_eq(cmd) || lh_math_eq(cmd->print, 0) || authum_null_eq(line))
    {
        return;
    }
    len = strlen(line);
    if (lh_math_gt(len, lh_cast_static(authum_usize_t, authum_command_line_max)))
    {
        len = lh_cast_static(authum_usize_t, authum_command_line_max);
    }
    lh_memory_std_copy(buf, line, len);
    buf[len] = lh_char_map_lf;
    buf[lh_math_add(len, 1u)] = lh_char_map_nul;
    cmd->print(cmd->out, buf);
}

void
authum_command_printf(const authum_command_t *cmd, const authum_char_t *fmt, ...)
{
    authum_char_t line[lh_math_add(authum_command_line_max, 1)];
    va_list ap;

    if (authum_null_eq(fmt))
    {
        return;
    }
    va_start(ap, fmt);
    authum_unused(vsnprintf(line, sizeof line, fmt, ap));
    va_end(ap);
    authum_command_print(cmd, line);
}

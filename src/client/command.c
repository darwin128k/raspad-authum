#include <authum/command.h>

#include <authum/cstr.h>
#include <authum/null.h>

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

    if (index >= authum_command_argc(cmd))
    {
        return "";
    }
    arg = cmd->argv[index];
    return authum_null_eq(arg) ? "" : arg;
}

authum_bool_t
authum_command_arg_i32(const authum_command_t *cmd, authum_usize_t index, authum_i32_t *out)
{
    if (index >= authum_command_argc(cmd))
    {
        return authum_bool_false;
    }
    return authum_cstr_parse_i32(authum_command_argv(cmd, index), out);
}

void
authum_command_print(const authum_command_t *cmd, const authum_char_t *line)
{
    authum_char_t buf[authum_command_line_max + 2];
    size_t len;

    if (authum_null_eq(cmd) || cmd->print == 0 || authum_null_eq(line))
    {
        return;
    }
    len = strlen(line);
    if (len > (size_t)authum_command_line_max)
    {
        len = (size_t)authum_command_line_max;
    }
    memcpy(buf, line, len);
    buf[len] = '\n';
    buf[len + 1u] = '\0';
    cmd->print(cmd->out, buf);
}

void
authum_command_printf(const authum_command_t *cmd, const authum_char_t *fmt, ...)
{
    authum_char_t line[authum_command_line_max + 1];
    va_list ap;

    if (authum_null_eq(fmt))
    {
        return;
    }
    va_start(ap, fmt);
    (void)vsnprintf(line, sizeof line, fmt, ap);
    va_end(ap);
    authum_command_print(cmd, line);
}

#include <authum/console.h>

#include <authum/commands/host.h>
#include <authum/cstr.h>
#include <authum/loader.h>
#include <authum/module.h>
#include <authum/module/info.h>
#include <authum/null.h>

#include <string.h>

void
authum_console_reply(authum_command_print_fn print, authum_ptr out, const authum_char_t *line)
{
    authum_command_t cmd;

    memset(&cmd, 0, sizeof cmd);
    cmd.size = sizeof cmd;
    cmd.print = print;
    cmd.out = out;
    authum_command_print(&cmd, line);
}

void
authum_console_list(authum_command_print_fn print, authum_ptr out)
{
    authum_command_t cmd;
    authum_module_t *child;
    const authum_module_info_t *info;

    memset(&cmd, 0, sizeof cmd);
    cmd.size = sizeof cmd;
    cmd.print = print;
    cmd.out = out;
    authum_command_printf(&cmd, "Authum modules (%u):",
                          (unsigned)authum_loader_get_loaded(&authum));
    for (child = authum_loader_get_first(&authum); !authum_null_eq(child);
         child = authum_loader_get_next(&authum, child)) {
        info = authum_module_get_info(child);
        if (authum_null_eq(info) || authum_null_eq(info->name)) {
            authum_command_print(&cmd, "  (unnamed module)");
            continue;
        }
        authum_command_printf(&cmd, "  %-16s %u.%u.%u%s", info->name, info->version.major,
                              info->version.minor, info->version.patch,
                              authum_commands_host_has_scope(info->name) ? "  (commands)" : "");
    }
    if (authum_loader_get_loaded(&authum) == 0u) {
        authum_command_print(&cmd, "  (none - check modules.list beside authum_mm.so)");
    }
    authum_command_print(&cmd, "authum help - all commands");
}

void
authum_console_help(authum_command_print_fn print, authum_ptr out)
{
    authum_console_reply(print, out, "authum                 - loaded modules");
    authum_console_reply(print, out, "authum version         - Authum version");
    authum_console_reply(print, out, "authum <module> [help] - commands of one module");
    if (authum_commands_host_print_help(authum_null, print, out) == 0u) {
        authum_console_reply(print, out, "  (no module commands)");
    }
}

void
authum_console_version(authum_command_print_fn print, authum_ptr out)
{
    authum_command_t cmd;
    const authum_version_t abi = AUTHUM_MODULE_ABI;

    memset(&cmd, 0, sizeof cmd);
    cmd.size = sizeof cmd;
    cmd.print = print;
    cmd.out = out;
    authum_command_printf(&cmd, "Authum %u.%u.%u (module ABI %u.%u.%u)",
                          authum_module_info.version.major, authum_module_info.version.minor,
                          authum_module_info.version.patch, abi.major, abi.minor, abi.patch);
}

void
authum_console_run(authum_usize_t argc, const authum_char_t *const *argv,
                   authum_command_print_fn print, authum_ptr out)
{
    const authum_char_t *scope;
    const authum_char_t *name;

    if (argc == 0u || authum_null_eq(argv) || authum_cstr_eq(argv[0], "list", authum_bool_true)) {
        authum_console_list(print, out);
        return;
    }
    if (authum_cstr_eq(argv[0], "help", authum_bool_true)) {
        authum_console_help(print, out);
        return;
    }
    if (authum_cstr_eq(argv[0], "version", authum_bool_true)) {
        authum_console_version(print, out);
        return;
    }

    scope = argv[0];
    name = (argc > 1u) ? argv[1] : "help";
    if (authum_commands_host_run(scope, name, (argc > 2u) ? argc - 2u : 0u,
                                 (argc > 2u) ? argv + 2 : authum_null, print, out)) {
        return;
    }
    if (authum_cstr_eq(name, "help", authum_bool_true) &&
        authum_commands_host_print_help(scope, print, out) != 0u) {
        return;
    }
    authum_console_reply(print, out,
                         authum_commands_host_has_scope(scope)
                             ? "authum: unknown command - try: authum <module> help"
                             : "authum: unknown module or no commands - try: authum help");
}

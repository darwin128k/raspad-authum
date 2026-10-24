#include <authum/console.h>

#include <authum/commands/host.h>
#include <authum/cstr.h>
#include <authum/loader.h>
#include <authum/module.h>
#include <authum/module/info.h>
#include <authum/null.h>

#include <lh/fs/path.h>
#include <lh/fs/path/style.h>
#include <lh/null.h>
#include <lh/os.h>
#include <lh/os/module.h>
#include <lh/str.h>
#include <lh/util/addr.h>

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
authum_console_fail(authum_command_print_fn print, authum_ptr out, const authum_char_t *what,
                    const authum_char_t *name)
{
    authum_command_t cmd;
    const lh_os_error_desc_ptr desc = lh_os_get_last_error_desc();

    memset(&cmd, 0, sizeof cmd);
    cmd.size = sizeof cmd;
    cmd.print = print;
    cmd.out = out;
    authum_command_printf(&cmd, "authum %s %s: FAILED (%s)", what, name,
                          desc != lh_null ? (const char *)desc : "unknown error");
}

void
authum_console_list(authum_command_print_fn print, authum_ptr out)
{
    authum_command_t cmd;
    authum_module_t *child;
    const authum_module_info_t *info;
    unsigned i = 0u;

    memset(&cmd, 0, sizeof cmd);
    cmd.size = sizeof cmd;
    cmd.print = print;
    cmd.out = out;
    authum_command_printf(&cmd,
                          "Authum modules (%u):", (unsigned)authum_loader_get_loaded(&authum));
    for (child = authum_loader_get_first(&authum); !authum_null_eq(child);
         child = authum_loader_get_next(&authum, child))
    {
        ++i;
        info = authum_module_get_info(child);
        if (authum_null_eq(info) || authum_null_eq(info->name))
        {
            authum_command_printf(&cmd, "  %u. (unnamed)  RUNNING", i);
            continue;
        }
        authum_command_printf(&cmd, "  %u. %-16s %u.%u.%u  RUNNING%s", i, info->name,
                              info->version.major, info->version.minor, info->version.patch,
                              authum_commands_host_has_scope(info->name) ? "  [commands]" : "");
    }
    if (authum_loader_get_loaded(&authum) == 0u)
    {
        authum_command_print(&cmd, "  (none — authum load <module> or modules.list)");
    }
    authum_command_print(&cmd, "authum help — all commands");
}

void
authum_console_help(authum_command_print_fn print, authum_ptr out)
{
    authum_console_reply(print, out, "authum / list / status     - loaded modules");
    authum_console_reply(print, out, "authum info [module]       - Authum or one module");
    authum_console_reply(print, out, "authum load <module>       - load from modules/");
    authum_console_reply(print, out, "authum unload <module>     - stop + unload (off)");
    authum_console_reply(print, out, "authum reload <module>     - unload then load");
    authum_console_reply(print, out, "authum version             - Authum + ABI");
    authum_console_reply(print, out, "authum <module> [help|…]   - module commands");
    if (authum_commands_host_print_help(authum_null, print, out) == 0u)
    {
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
authum_console_info(authum_command_print_fn print, authum_ptr out, const authum_char_t *name)
{
    authum_command_t cmd;
    authum_module_t *child;
    const authum_module_info_t *info;
    const lh_fs_path_t *image;
    lh_str_t scratch;
    const lh_str_ptr path_text;

    memset(&cmd, 0, sizeof cmd);
    cmd.size = sizeof cmd;
    cmd.print = print;
    cmd.out = out;

    if (authum_null_eq(name) || name[0] == '\0')
    {
        authum_console_version(print, out);
        authum_command_printf(&cmd, "modules loaded: %u",
                              (unsigned)authum_loader_get_loaded(&authum));
        authum_command_printf(&cmd, "module commands: %u", (unsigned)authum_commands_host_count());
        authum_command_print(&cmd, "use: authum info <module>");
        return;
    }

    child = authum_loader_find(&authum, name);
    if (authum_null_eq(child))
    {
        authum_command_printf(&cmd, "authum info %s: not loaded", name);
        return;
    }
    info = authum_module_get_info(child);
    if (authum_null_eq(info))
    {
        authum_command_printf(&cmd, "authum info %s: no info", name);
        return;
    }
    authum_command_printf(&cmd, "name      %s", info->name);
    authum_command_printf(&cmd, "version   %u.%u.%u", info->version.major, info->version.minor,
                          info->version.patch);
    authum_command_printf(&cmd, "abi       %u.%u.%u", info->abi.major, info->abi.minor,
                          info->abi.patch);
    if (!authum_null_eq(info->author) && info->author[0] != '\0')
    {
        authum_command_printf(&cmd, "author    %s", info->author);
    }
    if (!authum_null_eq(info->url) && info->url[0] != '\0')
    {
        authum_command_printf(&cmd, "url       %s", info->url);
    }
    authum_command_printf(&cmd, "status    %s",
                          lh_os_module_is_started(child) ? "RUNNING" : "STOPPED");
    authum_command_printf(&cmd, "commands  %s",
                          authum_commands_host_has_scope(info->name) ? "yes" : "no");
    image = lh_os_module_get_path(child);
    path_text = lh_null;
    if (lh_null_ne(image) && !lh_fs_path_is_empty(image))
    {
        path_text = lh_fs_path_to_cstr(image, lh_fs_path_style_posix, lh_addr_of(scratch));
        authum_command_printf(&cmd, "path      %s", path_text != lh_null ? path_text : "?");
        lh_str_deinit(lh_addr_of(scratch));
    }
}

void
authum_console_load(authum_command_print_fn print, authum_ptr out, const authum_char_t *name)
{
    authum_command_t cmd;

    memset(&cmd, 0, sizeof cmd);
    cmd.size = sizeof cmd;
    cmd.print = print;
    cmd.out = out;
    if (authum_null_eq(name) || name[0] == '\0')
    {
        authum_command_print(&cmd, "usage: authum load <module>");
        return;
    }
    if (authum_loader_load_name(&authum, name))
    {
        authum_command_printf(&cmd, "authum load %s: OK", name);
    }
    else
    {
        authum_console_fail(print, out, "load", name);
    }
}

void
authum_console_unload(authum_command_print_fn print, authum_ptr out, const authum_char_t *name)
{
    authum_command_t cmd;

    memset(&cmd, 0, sizeof cmd);
    cmd.size = sizeof cmd;
    cmd.print = print;
    cmd.out = out;
    if (authum_null_eq(name) || name[0] == '\0')
    {
        authum_command_print(&cmd, "usage: authum unload <module>");
        return;
    }
    if (authum_loader_unload_name(&authum, name))
    {
        authum_command_printf(&cmd, "authum unload %s: OK", name);
    }
    else
    {
        authum_console_fail(print, out, "unload", name);
    }
}

void
authum_console_reload(authum_command_print_fn print, authum_ptr out, const authum_char_t *name)
{
    authum_command_t cmd;

    memset(&cmd, 0, sizeof cmd);
    cmd.size = sizeof cmd;
    cmd.print = print;
    cmd.out = out;
    if (authum_null_eq(name) || name[0] == '\0')
    {
        authum_command_print(&cmd, "usage: authum reload <module>");
        return;
    }
    if (authum_loader_reload_name(&authum, name))
    {
        authum_command_printf(&cmd, "authum reload %s: OK", name);
    }
    else
    {
        authum_console_fail(print, out, "reload", name);
    }
}

void
authum_console_run(authum_usize_t argc, const authum_char_t *const *argv,
                   authum_command_print_fn print, authum_ptr out)
{
    const authum_char_t *scope;
    const authum_char_t *name;
    const authum_char_t *arg;

    if (argc == 0u || authum_null_eq(argv) || authum_cstr_eq(argv[0], "list", authum_bool_true) ||
        authum_cstr_eq(argv[0], "status", authum_bool_true))
    {
        authum_console_list(print, out);
        return;
    }
    if (authum_cstr_eq(argv[0], "help", authum_bool_true))
    {
        authum_console_help(print, out);
        return;
    }
    if (authum_cstr_eq(argv[0], "version", authum_bool_true))
    {
        authum_console_version(print, out);
        return;
    }
    if (authum_cstr_eq(argv[0], "info", authum_bool_true))
    {
        authum_console_info(print, out, argc > 1u ? argv[1] : authum_null);
        return;
    }
    if (authum_cstr_eq(argv[0], "load", authum_bool_true))
    {
        authum_console_load(print, out, argc > 1u ? argv[1] : authum_null);
        return;
    }
    if (authum_cstr_eq(argv[0], "unload", authum_bool_true))
    {
        authum_console_unload(print, out, argc > 1u ? argv[1] : authum_null);
        return;
    }
    if (authum_cstr_eq(argv[0], "reload", authum_bool_true))
    {
        authum_console_reload(print, out, argc > 1u ? argv[1] : authum_null);
        return;
    }

    scope = argv[0];
    name = (argc > 1u) ? argv[1] : "help";
    arg = (argc > 2u) ? argv[2] : authum_null;
    (void)arg;
    if (authum_commands_host_run(scope, name, (argc > 2u) ? argc - 2u : 0u,
                                 (argc > 2u) ? argv + 2 : authum_null, print, out))
    {
        return;
    }
    if (authum_cstr_eq(name, "help", authum_bool_true) &&
        authum_commands_host_print_help(scope, print, out) != 0u)
    {
        return;
    }
    authum_console_reply(print, out,
                         authum_commands_host_has_scope(scope)
                             ? "authum: unknown command - try: authum <module> help"
                             : "authum: unknown module or no commands - try: authum help");
}

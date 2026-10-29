#include <authum/host/console.h>
#include <lh/char/map.h>
#include <lh/memory/std.h>

#include <authum/host/commands.h>
#include <authum/cstr.h>
#include <authum/host/loader.h>
#include <authum/unused.h>

#include <lh/os.h>

#include <string.h>

void
authum_console_reply(authum_command_print_fn print, authum_ptr out, const authum_char_t *line)
{
    authum_command_t cmd;

    lh_memory_std_set(lh_addr_of(cmd), 0, sizeof cmd);
    cmd.print = print;
    cmd.out = out;
    authum_command_print(lh_addr_of(cmd), line);
}

void
authum_console_fail(authum_command_print_fn print, authum_ptr out, const authum_char_t *what,
                    const authum_char_t *name)
{
    authum_command_t cmd;
    const lh_os_error_desc_ptr desc = lh_os_get_last_error_desc();

    lh_memory_std_set(lh_addr_of(cmd), 0, sizeof cmd);
    cmd.print = print;
    cmd.out = out;
    authum_command_printf(lh_addr_of(cmd), "authum %s %s: FAILED (%s)", what, name,
                          lh_math_ne(desc, lh_null)
                              ? lh_cast_reinterpret(const authum_char_t *, desc)
                              : "unknown error");
}

void
authum_console_list(authum_command_print_fn print, authum_ptr out)
{
    authum_command_t cmd;
    authum_module_t *child;
    const authum_module_info_t *info;
    authum_u32_t i = 0u;

    lh_memory_std_set(lh_addr_of(cmd), 0, sizeof cmd);
    cmd.print = print;
    cmd.out = out;
    authum_command_printf(
        lh_addr_of(cmd), "Authum modules (%u):",
        lh_cast_static(authum_u32_t, authum_loader_get_loaded(lh_addr_of(authum))));
    for (child = authum_loader_get_first(lh_addr_of(authum)); !authum_null_eq(child);
         child = authum_loader_get_next(lh_addr_of(authum), child))
    {
        i = lh_math_add_one(i);
        info = authum_module_get_info(child);
        if (authum_null_eq(info) || authum_null_eq(info->name))
        {
            authum_command_printf(lh_addr_of(cmd), "  %u. (unnamed)  RUNNING", i);
            continue;
        }
        authum_command_printf(lh_addr_of(cmd), "  %u. %-16s %u.%u.%u  RUNNING%s", i, info->name,
                              info->version.major, info->version.minor, info->version.patch,
                              authum_commands_host_has_scope(info->name) ? "  [commands]" : "");
    }
    if (lh_math_eq(authum_loader_get_loaded(lh_addr_of(authum)), 0u))
    {
        authum_command_print(lh_addr_of(cmd), "  (none — authum load <module> or modules.list)");
    }
    authum_command_print(lh_addr_of(cmd), "authum help — all commands");
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
    if (lh_math_eq(authum_commands_host_print_help(authum_null, print, out), 0u))
    {
        authum_console_reply(print, out, "  (no module commands)");
    }
}

void
authum_console_version(authum_command_print_fn print, authum_ptr out)
{
    authum_command_t cmd;
    const authum_version_t abi = AUTHUM_MODULE_ABI;

    lh_memory_std_set(lh_addr_of(cmd), 0, sizeof cmd);
    cmd.print = print;
    cmd.out = out;
    authum_command_printf(lh_addr_of(cmd), "Authum %u.%u.%u (module ABI %u.%u.%u)",
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

    lh_memory_std_set(lh_addr_of(cmd), 0, sizeof cmd);
    cmd.print = print;
    cmd.out = out;

    if (authum_null_eq(name) || lh_math_eq(name[0], lh_char_map_nul))
    {
        authum_console_version(print, out);
        authum_command_printf(
            lh_addr_of(cmd), "modules loaded: %u",
            lh_cast_static(authum_u32_t, authum_loader_get_loaded(lh_addr_of(authum))));
        authum_command_printf(lh_addr_of(cmd), "module commands: %u",
                              lh_cast_static(authum_u32_t, authum_commands_host_count()));
        authum_command_print(lh_addr_of(cmd), "use: authum info <module>");
        return;
    }

    child = authum_loader_find(lh_addr_of(authum), name);
    if (authum_null_eq(child))
    {
        authum_command_printf(lh_addr_of(cmd), "authum info %s: not loaded", name);
        return;
    }
    info = authum_module_get_info(child);
    if (authum_null_eq(info))
    {
        authum_command_printf(lh_addr_of(cmd), "authum info %s: no info", name);
        return;
    }
    authum_command_printf(lh_addr_of(cmd), "name      %s", info->name);
    authum_command_printf(lh_addr_of(cmd), "version   %u.%u.%u", info->version.major,
                          info->version.minor, info->version.patch);
    authum_command_printf(lh_addr_of(cmd), "abi       %u.%u.%u", info->abi.major, info->abi.minor,
                          info->abi.patch);
    if (!authum_null_eq(info->author) && lh_math_ne(info->author[0], lh_char_map_nul))
    {
        authum_command_printf(lh_addr_of(cmd), "author    %s", info->author);
    }
    if (!authum_null_eq(info->url) && lh_math_ne(info->url[0], lh_char_map_nul))
    {
        authum_command_printf(lh_addr_of(cmd), "url       %s", info->url);
    }
    authum_command_printf(lh_addr_of(cmd), "status    %s",
                          lh_os_module_is_started(child) ? "RUNNING" : "STOPPED");
    authum_command_printf(lh_addr_of(cmd), "commands  %s",
                          authum_commands_host_has_scope(info->name) ? "yes" : "no");
    image = lh_os_module_get_path(child);
    path_text = lh_null;
    if (lh_null_ne(image) && !lh_fs_path_is_empty(image))
    {
        path_text = lh_fs_path_to_cstr(image, lh_fs_path_style_posix, lh_addr_of(scratch));
        authum_command_printf(lh_addr_of(cmd), "path      %s",
                              lh_math_ne(path_text, lh_null) ? path_text : "?");
        lh_str_deinit(lh_addr_of(scratch));
    }
}

void
authum_console_load(authum_command_print_fn print, authum_ptr out, const authum_char_t *name)
{
    authum_command_t cmd;

    lh_memory_std_set(lh_addr_of(cmd), 0, sizeof cmd);
    cmd.print = print;
    cmd.out = out;
    if (authum_null_eq(name) || lh_math_eq(name[0], lh_char_map_nul))
    {
        authum_command_print(lh_addr_of(cmd), "usage: authum load <module>");
        return;
    }
    if (authum_loader_load_name(lh_addr_of(authum), name))
    {
        authum_command_printf(lh_addr_of(cmd), "authum load %s: OK", name);
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

    lh_memory_std_set(lh_addr_of(cmd), 0, sizeof cmd);
    cmd.print = print;
    cmd.out = out;
    if (authum_null_eq(name) || lh_math_eq(name[0], lh_char_map_nul))
    {
        authum_command_print(lh_addr_of(cmd), "usage: authum unload <module>");
        return;
    }
    if (authum_loader_unload_name(lh_addr_of(authum), name))
    {
        authum_command_printf(lh_addr_of(cmd), "authum unload %s: OK", name);
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

    lh_memory_std_set(lh_addr_of(cmd), 0, sizeof cmd);
    cmd.print = print;
    cmd.out = out;
    if (authum_null_eq(name) || lh_math_eq(name[0], lh_char_map_nul))
    {
        authum_command_print(lh_addr_of(cmd), "usage: authum reload <module>");
        return;
    }
    if (authum_loader_reload_name(lh_addr_of(authum), name))
    {
        authum_command_printf(lh_addr_of(cmd), "authum reload %s: OK", name);
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

    if (lh_math_eq(argc, 0u) || authum_null_eq(argv) ||
        authum_cstr_eq(argv[0], "list", authum_bool_true) ||
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
        authum_console_info(print, out, lh_math_gt(argc, 1u) ? argv[1] : authum_null);
        return;
    }
    if (authum_cstr_eq(argv[0], "load", authum_bool_true))
    {
        authum_console_load(print, out, lh_math_gt(argc, 1u) ? argv[1] : authum_null);
        return;
    }
    if (authum_cstr_eq(argv[0], "unload", authum_bool_true))
    {
        authum_console_unload(print, out, lh_math_gt(argc, 1u) ? argv[1] : authum_null);
        return;
    }
    if (authum_cstr_eq(argv[0], "reload", authum_bool_true))
    {
        authum_console_reload(print, out, lh_math_gt(argc, 1u) ? argv[1] : authum_null);
        return;
    }

    scope = argv[0];
    name = (lh_math_gt(argc, 1u)) ? argv[1] : "help";
    arg = (lh_math_gt(argc, 2u)) ? argv[2] : authum_null;
    authum_unused(arg);
    if (authum_commands_host_run(scope, name, (lh_math_gt(argc, 2u)) ? lh_math_sub(argc, 2u) : 0u,
                                 (lh_math_gt(argc, 2u)) ? lh_math_add(argv, 2) : authum_null, print,
                                 out))
    {
        return;
    }
    if (authum_cstr_eq(name, "help", authum_bool_true) &&
        lh_math_ne(authum_commands_host_print_help(scope, print, out), 0u))
    {
        return;
    }
    authum_console_reply(print, out,
                         authum_commands_host_has_scope(scope)
                             ? "authum: unknown command - try: authum <module> help"
                             : "authum: unknown module or no commands - try: authum help");
}

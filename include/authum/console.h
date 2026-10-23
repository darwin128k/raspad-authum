/**
 * @file console.h
 * @brief The `authum` server command (host only).
 *
 * The one engine command Authum registers. Everything else is a sub-word:
 *
 * | Line | Does |
 * |------|------|
 * | `authum` / `authum list` / `authum status` | Loaded modules + running state |
 * | `authum info` | Authum itself |
 * | `authum info <module>` | One module (version, path, commands) |
 * | `authum load <module>` | Load `modules/<module>.so` |
 * | `authum unload <module>` | Stop + unload (temporary off) |
 * | `authum reload <module>` | Unload then load again |
 * | `authum help` | Built-ins + every module command |
 * | `authum version` | Authum + module ABI version |
 * | `authum <module> <name> [args…]` | Module command (<authum/commands.h>) |
 *
 * Output goes to the reply sink (`pfnServerPrint` in `authum_mm`), so an
 * RCON client sees it. Module commands disappear after `unload` and return
 * after a successful `load` / `reload` — Qproto's `info` included.
 */

#ifndef AUTHUM_CONSOLE_H
#define AUTHUM_CONSOLE_H

#include <authum/char.h>
#include <authum/command.h>
#include <authum/ptr.h>
#include <authum/size.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /** Run `authum argv…` (@p argv excludes the word `authum`). */
    void
    authum_console_run(authum_usize_t argc, const authum_char_t *const *argv,
                       authum_command_print_fn print, authum_ptr out);

    void
    authum_console_list(authum_command_print_fn print, authum_ptr out);

    void
    authum_console_help(authum_command_print_fn print, authum_ptr out);

    void
    authum_console_version(authum_command_print_fn print, authum_ptr out);

    void
    authum_console_info(authum_command_print_fn print, authum_ptr out, const authum_char_t *name);

    void
    authum_console_load(authum_command_print_fn print, authum_ptr out, const authum_char_t *name);

    void
    authum_console_unload(authum_command_print_fn print, authum_ptr out, const authum_char_t *name);

    void
    authum_console_reload(authum_command_print_fn print, authum_ptr out, const authum_char_t *name);

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_CONSOLE_H */

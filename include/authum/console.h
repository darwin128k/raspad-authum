/**
 * @file console.h
 * @brief The `authum` server command (host only).
 *
 * The one engine command Authum registers. Everything else is a sub-word:
 *
 * | Line | Does |
 * |------|------|
 * | `authum` / `authum list` | Loaded modules |
 * | `authum help` | Built-ins + every module command |
 * | `authum version` | Authum + module ABI version |
 * | `authum <module>` / `authum <module> help` | That module's commands |
 * | `authum <module> <name> [args…]` | Module command (<authum/commands.h>) |
 *
 * Output goes to the reply sink (`pfnServerPrint` in `authum_mm`), so an
 * RCON client sees it.
 */

#ifndef AUTHUM_CONSOLE_H
#define AUTHUM_CONSOLE_H

#include <authum/char.h>
#include <authum/command.h>
#include <authum/ptr.h>
#include <authum/size.h>

#ifdef __cplusplus
extern "C" {
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

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_CONSOLE_H */

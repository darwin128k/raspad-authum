/**
 * @file command.h
 * @brief One console command call: arguments + reply sink.
 *
 * The engine knows exactly one server command, `authum`. A module command is
 * `authum <module> <name> [args…]` — typed in the server console or sent over
 * RCON. Authum parses the line and calls the module's ::authum_command_fn
 * with an ::authum_command_t; the handler reads arguments and prints the reply
 * through the functions below (the reply reaches the RCON client).
 *
 * Fields are read through the accessors only.
 */

#ifndef AUTHUM_COMMAND_H
#define AUTHUM_COMMAND_H

#include <authum/bool.h>
#include <authum/char.h>
#include <authum/extern.h>
#include <authum/ptr.h>
#include <authum/size.h>

enum
{
    /** Longest line ::authum_command_printf builds (longer output is cut). */
    authum_command_line_max = 512
};

AUTHUM_EXTERN_C_BEGIN

typedef void (*authum_command_print_fn)(authum_ptr out, const authum_char_t *text);

AUTHUM_EXTERN_C_END

/**
 * @struct authum_command
 * @brief Filled by the host for the duration of one handler call.
 */
struct authum_command
{
    /** Module part of the line (`qproto`). */
    const authum_char_t *scope;
    /** Command part of the line (`info`). */
    const authum_char_t *name;
    /** Arguments after the command name. */
    authum_usize_t argc;
    const authum_char_t *const *argv;
    authum_command_print_fn print;
    authum_ptr out;
};
typedef struct authum_command authum_command_t;

AUTHUM_EXTERN_C_BEGIN

/**
 * Handler. Return ::authum_bool_false for bad usage: the host then prints
 * `usage: authum <module> <name> <usage>`.
 */
typedef authum_bool_t (*authum_command_fn)(const authum_command_t *cmd, authum_ptr user);

const authum_char_t *
authum_command_get_scope(const authum_command_t *cmd);

const authum_char_t *
authum_command_get_name(const authum_command_t *cmd);

authum_usize_t
authum_command_get_argc(const authum_command_t *cmd);

/** Argument @p index (0 = first after the name); "" when out of range. */
const authum_char_t *
authum_command_get_argv(const authum_command_t *cmd, authum_usize_t index);

/** Argument @p index as i32; false when missing or not a number. */
authum_bool_t
authum_command_try_get_arg_i32(const authum_command_t *cmd, authum_usize_t index,
                               authum_i32_t *out);

/** Print one line (a newline is appended). */
void
authum_command_print(const authum_command_t *cmd, const authum_char_t *line);

/** printf-style ::authum_command_print (one line, up to ::authum_command_line_max). */
void
authum_command_printf(const authum_command_t *cmd, const authum_char_t *fmt, ...)
#if defined(__GNUC__)
    __attribute__((format(printf, 2, 3)))
#endif
    ;

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_COMMAND_H */

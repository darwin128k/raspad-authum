/**
 * @file def.h
 * @brief Description of one module command (what ::authum_commands_add takes).
 */

#ifndef AUTHUM_COMMAND_DEF_H
#define AUTHUM_COMMAND_DEF_H

#include <authum/command.h>

AUTHUM_EXTERN_C_BEGIN

enum
{
    /** Longest module or command name (letters, digits, `_`, `-`, `.`). */
    authum_command_word_max = 32,
    authum_command_usage_max = 64,
    authum_command_help_max = 128
};

/**
 * @struct authum_command_def
 * @brief `authum <module> <name>` → @p fn(cmd, @p user).
 *
 * Strings are copied by the host; the def may live on the stack.
 */
struct authum_command_def
{
    /** Command word (`info`). Case-insensitive at the console. */
    const authum_char_t *name;
    /** Argument synopsis for help / bad usage (`[#userid | index | name]`), may be null. */
    const authum_char_t *usage;
    /** One-line description for `authum help`, may be null. */
    const authum_char_t *help;
    authum_command_fn fn;
    authum_ptr user;
};
typedef struct authum_command_def authum_command_def_t;

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_COMMAND_DEF_H */

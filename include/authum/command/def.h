/**
 * @file def.h
 * @brief Description of one module command (what ::authum_commands_add takes).
 */

#ifndef AUTHUM_COMMAND_DEF_H
#define AUTHUM_COMMAND_DEF_H

#include <authum/command.h>

enum
{
    /** Longest module or command name plus one (letters, digits, `_`, `-`, `.`). */
    authum_command_word_max = 32
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

AUTHUM_EXTERN_C_BEGIN

/**
 * @brief The command word (level 1).
 *
 * @param self The command description; must not be ::authum_null (checked).
 */
const authum_char_t *
authum_command_def_get_name(const authum_command_def_t *self);

/**
 * @brief Store the command word (level 1).
 *
 * A level-1 setter: the value is stored as given and is not validated.
 *
 * @param self The command description; must not be ::authum_null (checked).
 * @param name The new value.
 */
void
authum_command_def_set_name(authum_command_def_t *self, const authum_char_t *name);

/**
 * @brief The argument synopsis, or ::authum_null for none (level 1).
 *
 * @param self The command description; must not be ::authum_null (checked).
 */
const authum_char_t *
authum_command_def_get_usage(const authum_command_def_t *self);

/**
 * @brief Store the argument synopsis, or ::authum_null for none (level 1).
 *
 * A level-1 setter: the value is stored as given and is not validated.
 *
 * @param self The command description; must not be ::authum_null (checked).
 * @param usage The new value.
 */
void
authum_command_def_set_usage(authum_command_def_t *self, const authum_char_t *usage);

/**
 * @brief The one line of help, or ::authum_null for none (level 1).
 *
 * @param self The command description; must not be ::authum_null (checked).
 */
const authum_char_t *
authum_command_def_get_help(const authum_command_def_t *self);

/**
 * @brief Store the one line of help, or ::authum_null for none (level 1).
 *
 * A level-1 setter: the value is stored as given and is not validated.
 *
 * @param self The command description; must not be ::authum_null (checked).
 * @param help The new value.
 */
void
authum_command_def_set_help(authum_command_def_t *self, const authum_char_t *help);

/**
 * @brief The handler (level 1).
 *
 * @param self The command description; must not be ::authum_null (checked).
 */
authum_command_fn
authum_command_def_get_fn(const authum_command_def_t *self);

/**
 * @brief Store the handler (level 1).
 *
 * A level-1 setter: the value is stored as given and is not validated.
 *
 * @param self The command description; must not be ::authum_null (checked).
 * @param fn The new value.
 */
void
authum_command_def_set_fn(authum_command_def_t *self, authum_command_fn fn);

/**
 * @brief The value the handler is given (level 1).
 *
 * @param self The command description; must not be ::authum_null (checked).
 */
authum_ptr
authum_command_def_get_user(const authum_command_def_t *self);

/**
 * @brief Store the value the handler is given (level 1).
 *
 * A level-1 setter: the value is stored as given and is not validated.
 *
 * @param self The command description; must not be ::authum_null (checked).
 * @param user The new value.
 */
void
authum_command_def_set_user(authum_command_def_t *self, authum_ptr user);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_COMMAND_DEF_H */

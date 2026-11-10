/**
 * @file commands.h
 * @brief Command registry inside `authum` (not for modules).
 */

#ifndef AUTHUM_HOST_COMMANDS_H
#define AUTHUM_HOST_COMMANDS_H

#include <authum/commands.h>
#include <lh/core/char/map.h>
#include <lh/core/console.h>
#include <lh/core/util/return.h>

AUTHUM_EXTERN_C_BEGIN

const authum_commands_ops_t lh_ptr_deref(authum_commands_host_ops(void));

/** Letters, digits, `_`, `-`, `.`; 1 … authum_command_word_max - 1 chars. */
authum_bool_t
authum_commands_host_is_word(const authum_char_t *text);

/**
 * Run `<scope> <name> argv…`. Prints usage when the handler returns false.
 * @return False when no such command exists (nothing printed).
 */
authum_bool_t
authum_commands_host_run(const authum_char_t *scope, const authum_char_t *name, authum_usize_t argc,
                         const authum_char_t *const *argv, authum_command_print_fn print,
                         authum_ptr out);

/** True when at least one command is registered under @p scope. */
authum_bool_t
authum_commands_host_has_scope(const authum_char_t *scope);

/**
 * Print `authum <scope> <name> <usage> — <help>` for every command of
 * @p scope (null → every scope). @return number of lines printed.
 */
authum_usize_t
authum_commands_host_print_help(const authum_char_t *scope, authum_command_print_fn print,
                                authum_ptr out);

void
authum_commands_host_drop_owner(authum_ptr owner);

authum_usize_t
authum_commands_host_count(void);

void
authum_commands_host_clear(void);

/**
 * @brief Authum commands host get console.
 */
lh_console_t *
authum_commands_host_get_console(void);

/**
 * @brief Authum commands host print entry.
 */
void
authum_commands_host_print_entry(const lh_console_entry_t *entry, authum_command_print_fn print,
                                 authum_ptr out, const authum_char_t *prefix);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_HOST_COMMANDS_H */

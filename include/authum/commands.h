/**
 * @file commands.h
 * @brief Console / RCON commands of a module, routed through `authum`.
 *
 * Modules never register engine commands (`pfnAddServerCommand`): another
 * plugin (AMX Mod X, …) may own the same word, and an engine command whose
 * code was unloaded crashes the server. Authum registers one engine command,
 * `authum`, and routes `authum <module> <name> …` to the module that added
 * `<name>`. The module part is the module's own `authum_module_info.name`;
 * a module cannot add commands under another module's name.
 *
 * Commands are dropped by the host after the module's `stop`.
 */

#ifndef AUTHUM_COMMANDS_H
#define AUTHUM_COMMANDS_H

#include <authum/commands/ops.h>

/**
 * @struct authum_commands
 * @brief Per-module handle: host ops + who is calling + its module word.
 */
struct authum_commands
{
    const authum_commands_ops_t *ops;
    authum_ptr owner;
    const authum_char_t *scope;
};
typedef struct authum_commands authum_commands_t;

AUTHUM_EXTERN_C_BEGIN

/**
 * Add `authum <scope> <def->name>`. False on a bad name, a missing handler,
 * or when the name is already taken in this scope.
 */
authum_bool_t
authum_commands_add(const authum_commands_t *self, const authum_command_def_t *def);

/** Add @p count defs; stops at (and returns false for) the first failure. */
authum_bool_t
authum_commands_add_all(const authum_commands_t *self, const authum_command_def_t *defs,
                        authum_usize_t count);

authum_bool_t
authum_commands_remove(const authum_commands_t *self, const authum_char_t *name);

/** The module word commands are added under (`qproto`), or null. */
const authum_char_t *
authum_commands_scope(const authum_commands_t *self);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_COMMANDS_H */

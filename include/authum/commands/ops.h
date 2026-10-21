/**
 * @file ops.h
 * @brief Host ops table behind ::authum_commands_t (filled by `authum_mm`).
 *
 * Modules call the `authum_commands_*` functions in <authum/commands.h>,
 * never these pointers.
 */

#ifndef AUTHUM_COMMANDS_OPS_H
#define AUTHUM_COMMANDS_OPS_H

#include <authum/bool.h>
#include <authum/char.h>
#include <authum/command/def.h>
#include <authum/ptr.h>

#ifdef __cplusplus
extern "C"
{
#endif

    struct authum_commands_ops
    {
        authum_bool_t (*add)(authum_ptr owner, const authum_char_t *scope,
                             const authum_command_def_t *def);
        authum_bool_t (*remove)(authum_ptr owner, const authum_char_t *scope,
                                const authum_char_t *name);
    };
    typedef struct authum_commands_ops authum_commands_ops_t;

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_COMMANDS_OPS_H */

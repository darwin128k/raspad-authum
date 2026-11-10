/**
 * @file private.h
 * @brief Private to `command.c`: the functions below its public API.
 *
 * Not installed: modules never see these.
 */

#ifndef AUTHUM_API_COMMAND_PRIVATE_H
#define AUTHUM_API_COMMAND_PRIVATE_H

#include <authum/command.h>
#include <lh/core/assert/static.h>
#include <lh/core/bool.h>
#include <lh/core/console/call.h>
#include <lh/core/util/offset.h>
#include <lh/core/util/return.h>

AUTHUM_EXTERN_C_BEGIN

/**
 * @brief The console call @p cmd is.
 */
const lh_console_call_t *
authum_command_get_call(const authum_command_t *cmd);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_API_COMMAND_PRIVATE_H */

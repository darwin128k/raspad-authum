/**
 * @file ops.h
 * @brief Log channel the host hands each module (::authum_host_t.logger).
 *
 * The module only says what happened and how bad it is; the host decides
 * where it goes (its file under `modules/<module>/logs/`, Authum's log, the server
 * console) from `authum.cfg`. Use it through <authum/logger.h>.
 */

#ifndef AUTHUM_LOGGER_OPS_H
#define AUTHUM_LOGGER_OPS_H

#include <authum/bool.h>
#include <authum/extern.h>
#include <authum/logger/level.h>
#include <authum/logger/options.h>
#include <authum/ptr.h>

struct authum_logger_ops
{
    /** Record one line (no trailing newline) at @p level for @p owner. */
    void (*write)(authum_ptr owner, authum_logger_level_t level, const authum_char_t *text);
    /** Whether a line at @p level would go anywhere — skip formatting otherwise. */
    authum_bool_t (*is_enabled)(authum_ptr owner, authum_logger_level_t level);
    /**
     * Open the module's journal @p name (`modules/<module>/logs/<name>/`); the
     * result is the owner for `write` / `is_enabled`. Null on failure.
     */
    authum_ptr (*open)(authum_ptr owner, const authum_char_t *name);
    /** Close a journal from `open` (each open needs one close). */
    void (*close)(authum_ptr journal);
    /** ABI 1.4: construct a named logger with host-owned formatting defaults. */
    authum_ptr (*construct)(authum_ptr owner, const authum_logger_options_t *options);
};
typedef struct authum_logger_ops authum_logger_ops_t;

/** Ops bound to the module they log for. */
struct authum_logger_channel
{
    const authum_logger_ops_t *ops;
    authum_ptr owner;
};
typedef struct authum_logger_channel authum_logger_channel_t;

AUTHUM_EXTERN_C_BEGIN

/** @brief Level-1 read of ops; self must be nonnull. */
const authum_logger_ops_t *
authum_logger_channel_get_ops(const authum_logger_channel_t *self);

/** @brief Level-1 store of ops; no validation or ownership transfer. */
void
authum_logger_channel_set_ops(authum_logger_channel_t *self, const authum_logger_ops_t *value);

/** @brief Level-1 read of owner; self must be nonnull. */
authum_ptr
authum_logger_channel_get_owner(const authum_logger_channel_t *self);

/** @brief Level-1 store of owner; no validation or ownership transfer. */
void
authum_logger_channel_set_owner(authum_logger_channel_t *self, authum_ptr value);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_LOGGER_OPS_H */

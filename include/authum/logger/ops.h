/**
 * @file ops.h
 * @brief Log channel the host hands each module (::authum_host_t.logger).
 *
 * The module only says what happened and how bad it is; the host decides
 * where it goes (its file under `logs/<module>/`, `authum.log`, the server
 * console) from `authum.cfg`. Use it through <authum/logger.h>.
 */

#ifndef AUTHUM_LOGGER_OPS_H
#define AUTHUM_LOGGER_OPS_H

#include <authum/bool.h>
#include <authum/char.h>
#include <authum/logger/level.h>
#include <authum/ptr.h>
#include <authum/size.h>

#ifdef __cplusplus
extern "C"
{
#endif

    struct authum_logger_ops
    {
        /** sizeof this table as built by the host — see <authum/abi.h>. */
        authum_usize_t size;
        /** Record one line (no trailing newline) at @p level for @p owner. */
        void (*write)(authum_ptr owner, authum_logger_level_t level, const authum_char_t *text);
        /** Whether a line at @p level would go anywhere — skip formatting otherwise. */
        authum_bool_t (*is_enabled)(authum_ptr owner, authum_logger_level_t level);
    };
    typedef struct authum_logger_ops authum_logger_ops_t;

    /** Ops bound to the module they log for. */
    struct authum_logger_channel
    {
        const authum_logger_ops_t *ops;
        authum_ptr owner;
    };
    typedef struct authum_logger_channel authum_logger_channel_t;

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_LOGGER_OPS_H */

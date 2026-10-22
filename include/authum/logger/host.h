/**
 * @file host.h
 * @brief Host side of the log channels — implemented in `authum_mm` only.
 *
 * One registry of channels: the host's own (`authum`, owner null) and one per
 * loaded module. Each channel writes `<root>/logs/<name>/YYYY-MM-DD.log`
 * (a new file when the date changes). Levels come from `<root>/authum.cfg`:
 *
 * ```
 * log_level = info          # what reaches the files
 * log_console = warning     # what also goes to the server console / Metamod
 * qproto.log_level = debug  # one module (or `authum.log_level` for the host)
 * ```
 *
 * A module's warnings and worse are copied into the `authum` channel, tagged
 * `[module]`, so one file shows every problem on the server.
 */

#ifndef AUTHUM_LOGGER_HOST_H
#define AUTHUM_LOGGER_HOST_H

#include <authum/bool.h>
#include <authum/char.h>
#include <authum/logger/level.h>
#include <authum/logger/ops.h>
#include <authum/ptr.h>

#include <time.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /** Where console-level lines go besides the files (Metamod log / console). */
    typedef void (*authum_logs_mirror_fn)(authum_logger_level_t level, const authum_char_t *name,
                                          const authum_char_t *text);

    /** Wall clock; replaceable so tests can cross midnight. */
    typedef time_t (*authum_logs_clock_fn)(void);

    /**
     * Read `<root>/authum.cfg` and open the host channel. @p root is the
     * directory of `authum_mm` (`addons/authum`). @p mirror may be null.
     */
    authum_bool_t
    authum_logs_init(const authum_char_t *root, authum_logs_mirror_fn mirror);

    /** Close every channel. Lines written afterwards go to stderr. */
    void
    authum_logs_deinit(void);

    /** Ops for ::authum_logger_channel_t; owner is the module, null = host. */
    const authum_logger_ops_t *
    authum_logs_ops(void);

    /** Start a channel for module @p owner named @p name (its `log_level` is read now). */
    authum_bool_t
    authum_logs_open(authum_ptr owner, const authum_char_t *name);

    /** End @p owner's channel (after the module's `stop`). */
    void
    authum_logs_close(authum_ptr owner);

    void
    authum_logs_write(authum_ptr owner, authum_logger_level_t level, const authum_char_t *text);

    authum_bool_t
    authum_logs_is_enabled(authum_ptr owner, authum_logger_level_t level);

    void
    authum_logs_set_clock(authum_logs_clock_fn clock);

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_LOGGER_HOST_H */

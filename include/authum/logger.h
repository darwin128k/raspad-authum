/**
 * @file logger.h
 * @brief Module logger — PSR-3 levels; the host decides where lines go.
 *
 * ```c
 * authum_logger_t g_log;
 *
 * authum_logger_init(&g_log, &g_host);       // in start
 * authum_logger_info(&g_log, "loaded %d bans", count);
 * authum_logger_close(&g_log);               // in stop
 * ```
 *
 * The module never picks a file or a console: every line goes to the host
 * channel (::authum_host_t.logger), and `authum.cfg` routes it:
 *
 * - `addons/authum/logs/<module>/YYYY-MM-DD.log` — everything at or above the
 *   module's `log_level`;
 * - `addons/authum/logs/authum/YYYY-MM-DD.log` — the module's warnings and
 *   errors too, tagged `[module]`;
 * - server console / Metamod log — lines at or above `log_console`.
 *
 * Lines below the configured level are dropped before formatting, so
 * ::authum_logger_debug in a hot path costs one call when debug is off.
 */

#ifndef AUTHUM_LOGGER_H
#define AUTHUM_LOGGER_H

#include <authum/bool.h>
#include <authum/char.h>
#include <authum/host.h>
#include <authum/logger/level.h>
#include <authum/logger/ops.h>
#include <authum/null.h>
#include <authum/ptr.h>
#include <authum/size.h>

#include <stdarg.h>

#ifdef __cplusplus
extern "C"
{
#endif

    enum
    {
        /** Longest line after formatting; the rest is cut. */
        authum_logger_line_max = 1024
    };

    /**
     * @struct authum_logger
     * @brief Module-owned handle on the host's log channel.
     */
    struct authum_logger
    {
        const authum_logger_channel_t *channel;
    };
    typedef struct authum_logger authum_logger_t;

    /** Bind @p self to @p host's log channel (keep @p host alive until close). */
    void
    authum_logger_init(authum_logger_t *self, const authum_host_t *host);

    /** Drop the binding; later calls do nothing. */
    void
    authum_logger_close(authum_logger_t *self);

    /** Whether a line at @p level would be recorded anywhere. */
    authum_bool_t
    authum_logger_is_enabled(const authum_logger_t *self, authum_logger_level_t level);

    void
    authum_logger_log_v(authum_logger_t *self, authum_logger_level_t level,
                        const authum_char_t *fmt, va_list ap);

    void
    authum_logger_log(authum_logger_t *self, authum_logger_level_t level, const authum_char_t *fmt,
                      ...);

#define authum_logger_emergency(self, ...)                                                         \
    authum_logger_log((self), authum_logger_level_emergency, __VA_ARGS__)
#define authum_logger_alert(self, ...)                                                             \
    authum_logger_log((self), authum_logger_level_alert, __VA_ARGS__)
#define authum_logger_critical(self, ...)                                                          \
    authum_logger_log((self), authum_logger_level_critical, __VA_ARGS__)
#define authum_logger_error(self, ...)                                                             \
    authum_logger_log((self), authum_logger_level_error, __VA_ARGS__)
#define authum_logger_warning(self, ...)                                                           \
    authum_logger_log((self), authum_logger_level_warning, __VA_ARGS__)
#define authum_logger_notice(self, ...)                                                            \
    authum_logger_log((self), authum_logger_level_notice, __VA_ARGS__)
#define authum_logger_info(self, ...)                                                              \
    authum_logger_log((self), authum_logger_level_info, __VA_ARGS__)
#define authum_logger_debug(self, ...)                                                             \
    authum_logger_log((self), authum_logger_level_debug, __VA_ARGS__)

    /** Log @p len bytes of @p data as hex at debug level, 16 per line. */
    void
    authum_logger_hex_dump(authum_logger_t *self, const authum_char_t *label, authum_ptr data,
                           authum_usize_t len);

    /* --- channel bricks (what the logger and authum_host_log_line call) ------ */

    void
    authum_logger_channel_write(const authum_logger_channel_t *channel, authum_logger_level_t level,
                                const authum_char_t *text);

    authum_bool_t
    authum_logger_channel_is_enabled(const authum_logger_channel_t *channel,
                                     authum_logger_level_t level);

    /**
     * Level from its name (`debug`, `info`, `warning`, … as in PSR-3;
     * `warn` / `err` / `crit` accepted). False when @p name is none of them.
     */
    authum_bool_t
    authum_logger_level_parse(const authum_char_t *name, authum_logger_level_t *out);

    /** Lower-case PSR-3 name of @p level (`"warning"`), `"?"` when out of range. */
    const authum_char_t *
    authum_logger_level_name(authum_logger_level_t level);

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_LOGGER_H */

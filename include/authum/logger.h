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
 * channel (::authum_host_t.logger). Module cfg preferences inherit global
 * defaults; explicit per-module entries in `authum.cfg` override them:
 *
 * - `addons/authum/modules/<module>/logs/YYYY-MM-DD.log` — everything at or above the
 *   module's `log_level` when `log_file=on`;
 * - `addons/authum/logs/YYYY-MM-DD.log` (Authum's) — the module's warnings and
 *   errors too, tagged `[module]`;
 * - server console — lines at or above `log_console`.
 * `log_level=off` disables all destinations, including the module's journals.
 *
 * Lines below the configured level are dropped before formatting, so
 * ::authum_logger_debug in a hot path costs one call when debug is off.
 * During a logger-buffer capture scope, filtering is deferred to the game-thread flush.
 *
 * A task that wants its own file (bans, payments, an audit trail) opens a
 * journal with ::authum_logger_open — same calls, its own folder and level.
 */

#ifndef AUTHUM_LOGGER_H
#define AUTHUM_LOGGER_H

#include <authum/bool.h>
#include <authum/char.h>
#include <authum/extern.h>
#include <authum/host.h>
#include <authum/logger/level.h>
#include <authum/logger/ops.h>
#include <authum/null.h>
#include <authum/ptr.h>
#include <authum/size.h>

#include <stdarg.h>

AUTHUM_EXTERN_C_BEGIN

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
    /** The module's main channel (from the host). */
    const authum_logger_channel_t *channel;
    /** A journal opened by ::authum_logger_open. */
    authum_logger_channel_t journal;
    authum_bool_t owns_journal;
};
typedef struct authum_logger authum_logger_t;

/** Bind @p self to @p host's log channel (keep @p host alive until close). */
void
authum_logger_init(authum_logger_t *self, const authum_host_t *host);

/**
 * Open journal @p name of this module: its own folder
 * `modules/<module>/logs/<name>/YYYY-MM-DD.log`, its own level
 * (`<module>.<name>.log_level`, else the module's). Warnings and errors
 * still reach the Authum log, tagged `[module/name]`. Use it like any
 * logger; close it in `stop`. False when the host has no journals.
 *
 * ```c
 * authum_logger_open(&g_bans, &g_host, "bans");
 * authum_logger_info(&g_bans, "%s banned %s", admin, target);
 * ```
 */
authum_bool_t
authum_logger_open(authum_logger_t *self, const authum_host_t *host, const authum_char_t *name);

/** Construct a special logger; defaults stay overridable by configuration.
 * False when the host lacks the ABI 1.4 constructor; no silent format fallback. */
authum_bool_t
authum_logger_create(authum_logger_t *self, const authum_host_t *host,
                     const authum_logger_options_t *options);

/** Use the module's automatically supplied channel, without a logger handle. */
void
authum_host_log(const authum_host_t *host, authum_logger_level_t level, const authum_char_t *fmt,
                ...);

#define authum_host_log_info(host, ...)                                                            \
    authum_host_log((host), authum_logger_level_info, __VA_ARGS__)
#define authum_host_log_warning(host, ...)                                                         \
    authum_host_log((host), authum_logger_level_warning, __VA_ARGS__)
#define authum_host_log_error(host, ...)                                                           \
    authum_host_log((host), authum_logger_level_error, __VA_ARGS__)
#define authum_host_log_debug(host, ...)                                                           \
    authum_host_log((host), authum_logger_level_debug, __VA_ARGS__)

/** Drop the binding (and close a journal); later calls do nothing. */
void
authum_logger_close(authum_logger_t *self);

/** Whether a line at @p level would be recorded anywhere. */
authum_bool_t
authum_logger_is_enabled(const authum_logger_t *self, authum_logger_level_t level);

void
authum_logger_log_v(authum_logger_t *self, authum_logger_level_t level, const authum_char_t *fmt,
                    va_list ap);

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

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_LOGGER_H */

/**
 * @file logger.h
 * @brief Standard module logger — lh-shaped levels, Authum sinks (no lh link).
 *
 * Modules own an ::authum_logger_t (stack or static). Wire it once from
 * `start` with the host + sink mask; then use ::authum_logger_info / `_error`
 * / … . Want something else? Write your own, or call the same bricks
 * (`authum_host_log_line`, ::authum_host_server_print, `FILE *`) yourself.
 *
 * Not a process-wide singleton: each module (and each logger instance) is
 * independent. The host’s `authum.log` is only reached through
 * ::authum_logger_sink_host.
 */

#ifndef AUTHUM_LOGGER_H
#define AUTHUM_LOGGER_H

#include <authum/bool.h>
#include <authum/char.h>
#include <authum/engine.h>
#include <authum/host.h>
#include <authum/logger/file/split.h>
#include <authum/logger/level.h>
#include <authum/logger/level/flags.h>
#include <authum/logger/sink.h>
#include <authum/null.h>
#include <authum/path.h>
#include <authum/ptr.h>
#include <authum/size.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef __cplusplus
extern "C"
{
#endif

    enum
    {
        authum_logger_line_max = 1024
    };

    /**
     * @struct authum_logger
     * @brief Module-owned logger: level filter + sink mask + optional file.
     */
    struct authum_logger
    {
        const authum_host_t *host;
        authum_logger_sink_t sinks;
        authum_logger_level_flags_t level_flags;
        /** Short name for prefixes (`qproto` → `[qproto] …` on host / console). */
        const authum_char_t *tag;
        FILE *file;
    };
    typedef struct authum_logger authum_logger_t;

    void
    authum_logger_init(authum_logger_t *self, const authum_host_t *host, authum_logger_sink_t sinks,
                       const authum_char_t *tag);

    void
    authum_logger_set_level_flags(authum_logger_t *self, authum_logger_level_flags_t flags);

    void
    authum_logger_set_sinks(authum_logger_t *self, authum_logger_sink_t sinks);

    /**
     * @brief Open/append a private transcript. Enables nothing by itself — set
     *        ::authum_logger_sink_file in @c sinks.
     */
    authum_bool_t
    authum_logger_open_file(authum_logger_t *self, const authum_char_t *path);

    /**
     * @brief Create @p dir if missing (one level). Idempotent when it already exists.
     */
    authum_bool_t
    authum_logger_ensure_dir(const authum_char_t *dir);

    /**
     * @brief Build a private log path under ::authum_host_env_module.
     *
     * @p tag is the file stem (`qproto` → `qproto.log` / `logs/qproto-….log`).
     * Empty/null @p tag falls back to `"module"`.
     */
    authum_bool_t
    authum_logger_build_module_path(const authum_host_t *host, const authum_char_t *tag,
                                    authum_logger_file_split_t split, authum_path_t *out);

    /**
     * @brief Open a private file via ::authum_logger_build_module_path using
     *        @c self->host and @c self->tag. Optional @p out receives the path.
     */
    authum_bool_t
    authum_logger_open_module_file(authum_logger_t *self, authum_logger_file_split_t split,
                                   authum_path_t *out);

    void
    authum_logger_close(authum_logger_t *self);

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

    void
    authum_logger_hex_dump(authum_logger_t *self, const authum_char_t *label, authum_ptr data,
                           authum_usize_t len);

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_LOGGER_H */

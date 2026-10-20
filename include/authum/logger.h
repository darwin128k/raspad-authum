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
#include <authum/logger/level.h>
#include <authum/logger/level/flags.h>
#include <authum/logger/sink.h>
#include <authum/null.h>
#include <authum/ptr.h>
#include <authum/size.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
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

static inline void
authum_logger_init(authum_logger_t *self, const authum_host_t *host, authum_logger_sink_t sinks,
                   const authum_char_t *tag)
{
    if (authum_null_eq(self)) {
        return;
    }
    self->host = host;
    self->sinks = sinks;
    self->level_flags = (authum_logger_level_flags_t)authum_logger_level_flags_all;
    self->tag = tag;
    self->file = authum_null;
}

static inline void
authum_logger_set_level_flags(authum_logger_t *self, authum_logger_level_flags_t flags)
{
    if (!authum_null_eq(self)) {
        self->level_flags = flags;
    }
}

static inline void
authum_logger_set_sinks(authum_logger_t *self, authum_logger_sink_t sinks)
{
    if (!authum_null_eq(self)) {
        self->sinks = sinks;
    }
}

/**
 * @brief Open/append a private transcript. Enables nothing by itself — set
 *        ::authum_logger_sink_file in @c sinks.
 */
static inline authum_bool_t
authum_logger_open_file(authum_logger_t *self, const authum_char_t *path)
{
    if (authum_null_eq(self) || authum_null_eq(path) || path[0] == '\0') {
        return authum_bool_false;
    }
    if (!authum_null_eq(self->file)) {
        fclose(self->file);
        self->file = authum_null;
    }
    self->file = fopen(path, "a");
    if (authum_null_eq(self->file)) {
        return authum_bool_false;
    }
    setvbuf(self->file, authum_null, _IONBF, 0);
    {
        time_t t = time(authum_null);
        fprintf(self->file, "\n---- log opened %s", ctime(&t));
    }
    return authum_bool_true;
}

static inline void
authum_logger_close(authum_logger_t *self)
{
    if (authum_null_eq(self)) {
        return;
    }
    if (!authum_null_eq(self->file)) {
        fclose(self->file);
        self->file = authum_null;
    }
    self->host = authum_null;
    self->sinks = authum_logger_sink_none;
}

static inline void
authum_logger_log_v(authum_logger_t *self, authum_logger_level_t level, const authum_char_t *fmt,
                    va_list ap)
{
    authum_char_t buf[authum_logger_line_max];
    authum_char_t host_line[authum_logger_line_max + 64];
    authum_char_t console_line[authum_logger_line_max + 64];
    int is_error;

    if (authum_null_eq(self) || authum_null_eq(fmt)) {
        return;
    }
    if ((self->level_flags & (authum_logger_level_flags_t)(1u << level)) == 0u) {
        return;
    }
    if ((self->sinks & (authum_logger_sink_console | authum_logger_sink_file |
                        authum_logger_sink_host)) == 0u) {
        return;
    }

    vsnprintf(buf, sizeof buf, fmt, ap);
    is_error = (level <= authum_logger_level_error) ? 1 : 0;

    if ((self->sinks & authum_logger_sink_host) != 0u && !authum_null_eq(self->host)) {
        if (!authum_null_eq(self->tag) && self->tag[0] != '\0') {
            snprintf(host_line, sizeof host_line, "[%s] %s", self->tag, buf);
            authum_host_log_line(self->host, is_error, host_line);
        } else {
            authum_host_log_line(self->host, is_error, buf);
        }
    }

    if ((self->sinks & authum_logger_sink_file) != 0u && !authum_null_eq(self->file)) {
        time_t t = time(authum_null);
        struct tm *lt = localtime(&t);
        if (!authum_null_eq(lt)) {
            fprintf(self->file, "[%02d:%02d:%02d] %s\n", lt->tm_hour, lt->tm_min, lt->tm_sec, buf);
        } else {
            fprintf(self->file, "%s\n", buf);
        }
    }

    if ((self->sinks & authum_logger_sink_console) != 0u && !authum_null_eq(self->host) &&
        !authum_null_eq(authum_host_engine(self->host))) {
        if (!authum_null_eq(self->tag) && self->tag[0] != '\0') {
            snprintf(console_line, sizeof console_line, "[%s] %s\n", self->tag, buf);
        } else {
            snprintf(console_line, sizeof console_line, "%s\n", buf);
        }
        authum_host_server_print(self->host, console_line);
    }
}

static inline void
authum_logger_log(authum_logger_t *self, authum_logger_level_t level, const authum_char_t *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    authum_logger_log_v(self, level, fmt, ap);
    va_end(ap);
}

#define authum_logger_emergency(self, ...)                                                         \
    authum_logger_log((self), authum_logger_level_emergency, __VA_ARGS__)
#define authum_logger_alert(self, ...) authum_logger_log((self), authum_logger_level_alert, __VA_ARGS__)
#define authum_logger_critical(self, ...)                                                          \
    authum_logger_log((self), authum_logger_level_critical, __VA_ARGS__)
#define authum_logger_error(self, ...) authum_logger_log((self), authum_logger_level_error, __VA_ARGS__)
#define authum_logger_warning(self, ...)                                                           \
    authum_logger_log((self), authum_logger_level_warning, __VA_ARGS__)
#define authum_logger_notice(self, ...)                                                            \
    authum_logger_log((self), authum_logger_level_notice, __VA_ARGS__)
#define authum_logger_info(self, ...) authum_logger_log((self), authum_logger_level_info, __VA_ARGS__)
#define authum_logger_debug(self, ...) authum_logger_log((self), authum_logger_level_debug, __VA_ARGS__)

static inline void
authum_logger_hex_dump(authum_logger_t *self, const authum_char_t *label, authum_ptr data,
                       authum_usize_t len)
{
    const authum_byte_t *p;
    authum_char_t line[128];
    authum_usize_t i;

    if (authum_null_eq(self) || authum_null_eq(label)) {
        return;
    }
    if (authum_null_eq(data) && len != 0u) {
        return;
    }

    p = (const authum_byte_t *)data;
    authum_logger_info(self, "%s (%u bytes):", label, (unsigned)len);

    for (i = 0u; i < len; i += 16u) {
        int pos = snprintf(line, sizeof line, "  %04x: ", (unsigned)i);
        authum_usize_t j;
        for (j = 0u; j < 16u && (i + j) < len; ++j) {
            pos += snprintf(line + pos, sizeof line - (authum_usize_t)pos, "%02x ", p[i + j]);
        }
        authum_logger_info(self, "%s", line);
    }
}

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_LOGGER_H */

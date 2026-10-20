#include <authum/logger.h>

void
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

void
authum_logger_set_level_flags(authum_logger_t *self, authum_logger_level_flags_t flags)
{
    if (!authum_null_eq(self)) {
        self->level_flags = flags;
    }
}

void
authum_logger_set_sinks(authum_logger_t *self, authum_logger_sink_t sinks)
{
    if (!authum_null_eq(self)) {
        self->sinks = sinks;
    }
}

authum_bool_t
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

void
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

void
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

void
authum_logger_log(authum_logger_t *self, authum_logger_level_t level, const authum_char_t *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    authum_logger_log_v(self, level, fmt, ap);
    va_end(ap);
}

void
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

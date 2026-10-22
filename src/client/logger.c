#include <authum/logger.h>

#include <authum/cstr.h>

#include "os/os.h"

authum_logger_file_split_t
authum_logger_file_split_parse(const authum_char_t *value)
{
    if (authum_cstr_eq(value, "session", authum_bool_true) ||
        authum_cstr_eq(value, "sessions", authum_bool_true) ||
        authum_cstr_eq(value, "1", authum_bool_true))
    {
        return authum_logger_file_split_session;
    }
    return authum_logger_file_split_full;
}

authum_bool_t
authum_logger_ensure_dir(const authum_char_t *dir)
{
    if (authum_null_eq(dir) || dir[0] == '\0')
    {
        return authum_bool_false;
    }
    return authum_os_dir_ensure(dir);
}

authum_bool_t
authum_logger_build_module_path(const authum_host_t *host, const authum_char_t *tag,
                                authum_logger_file_split_t split, authum_path_t *out)
{
    authum_path_t logs_dir;
    authum_char_t leaf[64];
    const authum_char_t *stem;
    time_t now;
    struct tm *tm;

    if (authum_null_eq(host) || authum_null_eq(out))
    {
        return authum_bool_false;
    }

    stem = (authum_null_eq(tag) || tag[0] == '\0') ? "module" : tag;

    if (split == authum_logger_file_split_session)
    {
        if (!authum_path_join(&logs_dir, authum_host_env_module(host), "logs"))
        {
            return authum_bool_false;
        }
        if (!authum_logger_ensure_dir(authum_path_as_cstr(&logs_dir)))
        {
            return authum_bool_false;
        }
        now = time(NULL);
        tm = localtime(&now);
        if (tm == NULL)
        {
            snprintf(leaf, sizeof leaf, "%s-%ld.log", stem, (long)now);
        }
        else
        {
            snprintf(leaf, sizeof leaf, "%s-%04d%02d%02d-%02d%02d%02d.log", stem,
                     tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min,
                     tm->tm_sec);
        }
        return authum_path_join(out, &logs_dir, leaf) ? authum_bool_true : authum_bool_false;
    }

    snprintf(leaf, sizeof leaf, "%s.log", stem);
    return authum_path_join(out, authum_host_env_module(host), leaf) ? authum_bool_true
                                                                     : authum_bool_false;
}

authum_bool_t
authum_logger_open_module_file(authum_logger_t *self, authum_logger_file_split_t split,
                               authum_path_t *out)
{
    authum_path_t path;
    authum_path_t *dest;

    if (authum_null_eq(self) || authum_null_eq(self->host))
    {
        return authum_bool_false;
    }

    dest = authum_null_eq(out) ? &path : out;
    if (!authum_logger_build_module_path(self->host, self->tag, split, dest))
    {
        return authum_bool_false;
    }
    return authum_logger_open_file(self, authum_path_as_cstr(dest));
}

void
authum_logger_init(authum_logger_t *self, const authum_host_t *host, authum_logger_sink_t sinks,
                   const authum_char_t *tag)
{
    if (authum_null_eq(self))
    {
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
    if (!authum_null_eq(self))
    {
        self->level_flags = flags;
    }
}

void
authum_logger_set_sinks(authum_logger_t *self, authum_logger_sink_t sinks)
{
    if (!authum_null_eq(self))
    {
        self->sinks = sinks;
    }
}

authum_bool_t
authum_logger_open_file(authum_logger_t *self, const authum_char_t *path)
{
    if (authum_null_eq(self) || authum_null_eq(path) || path[0] == '\0')
    {
        return authum_bool_false;
    }
    if (!authum_null_eq(self->file))
    {
        fclose(self->file);
        self->file = authum_null;
    }
    self->file = fopen(path, "a");
    if (authum_null_eq(self->file))
    {
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
    if (authum_null_eq(self))
    {
        return;
    }
    if (!authum_null_eq(self->file))
    {
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

    if (authum_null_eq(self) || authum_null_eq(fmt))
    {
        return;
    }
    if ((self->level_flags & (authum_logger_level_flags_t)(1u << level)) == 0u)
    {
        return;
    }
    if ((self->sinks &
         (authum_logger_sink_console | authum_logger_sink_file | authum_logger_sink_host)) == 0u)
    {
        return;
    }

    vsnprintf(buf, sizeof buf, fmt, ap);
    is_error = (level <= authum_logger_level_error) ? 1 : 0;

    if ((self->sinks & authum_logger_sink_host) != 0u && !authum_null_eq(self->host))
    {
        if (!authum_null_eq(self->tag) && self->tag[0] != '\0')
        {
            snprintf(host_line, sizeof host_line, "[%s] %s", self->tag, buf);
            authum_host_log_line(self->host, is_error, host_line);
        }
        else
        {
            authum_host_log_line(self->host, is_error, buf);
        }
    }

    if ((self->sinks & authum_logger_sink_file) != 0u && !authum_null_eq(self->file))
    {
        time_t t = time(authum_null);
        struct tm *lt = localtime(&t);
        if (!authum_null_eq(lt))
        {
            fprintf(self->file, "[%02d:%02d:%02d] %s\n", lt->tm_hour, lt->tm_min, lt->tm_sec, buf);
        }
        else
        {
            fprintf(self->file, "%s\n", buf);
        }
    }

    if ((self->sinks & authum_logger_sink_console) != 0u && !authum_null_eq(self->host) &&
        !authum_null_eq(authum_host_engine(self->host)))
    {
        if (!authum_null_eq(self->tag) && self->tag[0] != '\0')
        {
            snprintf(console_line, sizeof console_line, "[%s] %s\n", self->tag, buf);
        }
        else
        {
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

    if (authum_null_eq(self) || authum_null_eq(label))
    {
        return;
    }
    if (authum_null_eq(data) && len != 0u)
    {
        return;
    }

    p = (const authum_byte_t *)data;
    authum_logger_info(self, "%s (%u bytes):", label, (unsigned)len);

    for (i = 0u; i < len; i += 16u)
    {
        int pos = snprintf(line, sizeof line, "  %04x: ", (unsigned)i);
        authum_usize_t j;
        for (j = 0u; j < 16u && (i + j) < len; ++j)
        {
            pos += snprintf(line + pos, sizeof line - (authum_usize_t)pos, "%02x ", p[i + j]);
        }
        authum_logger_info(self, "%s", line);
    }
}

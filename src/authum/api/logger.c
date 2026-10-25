#include <authum/logger.h>
#include <authum/logger/buffer.h>

#include <authum/abi.h>
#include <authum/cstr.h>

#include <stdio.h>
#include <string.h>

static const struct
{
    const authum_char_t *name;
    authum_logger_level_t level;
} authum_logger_level_names[] = {
    {"emergency", authum_logger_level_emergency}, {"alert", authum_logger_level_alert},
    {"critical", authum_logger_level_critical},   {"error", authum_logger_level_error},
    {"warning", authum_logger_level_warning},     {"notice", authum_logger_level_notice},
    {"info", authum_logger_level_info},           {"debug", authum_logger_level_debug},
    {"crit", authum_logger_level_critical},       {"err", authum_logger_level_error},
    {"warn", authum_logger_level_warning},
};

authum_bool_t
authum_logger_level_parse(const authum_char_t *name, authum_logger_level_t *out)
{
    authum_usize_t i;

    if (authum_null_eq(name) || authum_null_eq(out))
    {
        return authum_bool_false;
    }
    for (i = 0u; i < sizeof authum_logger_level_names / sizeof authum_logger_level_names[0]; ++i)
    {
        if (authum_cstr_eq(name, authum_logger_level_names[i].name, authum_bool_true))
        {
            *out = authum_logger_level_names[i].level;
            return authum_bool_true;
        }
    }
    return authum_bool_false;
}

const authum_char_t *
authum_logger_level_name(authum_logger_level_t level)
{
    return level <= authum_logger_level_debug ? authum_logger_level_names[level].name : "?";
}

void
authum_logger_channel_write(const authum_logger_channel_t *channel, authum_logger_level_t level,
                            const authum_char_t *text)
{
    if (!authum_null_eq(channel) && !authum_null_eq(channel->ops) &&
        AUTHUM_ABI_HAS(channel->ops, write) && !authum_null_eq(text))
    {
        channel->ops->write(channel->owner, level, text);
    }
}

authum_bool_t
authum_logger_channel_is_enabled(const authum_logger_channel_t *channel,
                                 authum_logger_level_t level)
{
    if (authum_null_eq(channel) || authum_null_eq(channel->ops) ||
        !AUTHUM_ABI_HAS(channel->ops, write))
    {
        return authum_bool_false;
    }
    /* A host without the filter takes every line. */
    return AUTHUM_ABI_HAS(channel->ops, is_enabled)
               ? channel->ops->is_enabled(channel->owner, level)
               : authum_bool_true;
}

/** The channel @p self logs to: its journal, else the module's. */
const authum_logger_channel_t *
authum_logger_get_channel(const authum_logger_t *self)
{
    if (authum_null_eq(self))
    {
        return authum_null;
    }
    return self->owns_journal ? &self->journal : self->channel;
}

void
authum_logger_init(authum_logger_t *self, const authum_host_t *host)
{
    if (!authum_null_eq(self))
    {
        memset(self, 0, sizeof *self);
        self->channel = authum_host_logger(host);
    }
}

authum_bool_t
authum_logger_open(authum_logger_t *self, const authum_host_t *host, const authum_char_t *name)
{
    const authum_logger_channel_t *module;
    authum_ptr journal;

    if (authum_null_eq(self))
    {
        return authum_bool_false;
    }
    authum_logger_init(self, host);
    module = self->channel;
    if (authum_null_eq(module) || authum_null_eq(module->ops) ||
        !AUTHUM_ABI_HAS(module->ops, open) || !AUTHUM_ABI_HAS(module->ops, close) ||
        authum_null_eq(name) || name[0] == '\0')
    {
        self->channel = authum_null;
        return authum_bool_false;
    }
    journal = module->ops->open(module->owner, name);
    if (authum_null_eq(journal))
    {
        self->channel = authum_null;
        return authum_bool_false;
    }
    self->journal.ops = module->ops;
    self->journal.owner = journal;
    self->owns_journal = authum_bool_true;
    return authum_bool_true;
}

void
authum_logger_close(authum_logger_t *self)
{
    if (authum_null_eq(self))
    {
        return;
    }
    if (self->owns_journal)
    {
        self->journal.ops->close(self->journal.owner);
    }
    memset(self, 0, sizeof *self);
}

authum_bool_t
authum_logger_is_enabled(const authum_logger_t *self, authum_logger_level_t level)
{
    if (authum_logger_buffer_active()) return self && level <= authum_logger_level_debug;
    return authum_logger_channel_is_enabled(authum_logger_get_channel(self), level);
}

void
authum_logger_log_v(authum_logger_t *self, authum_logger_level_t level, const authum_char_t *fmt,
                    va_list ap)
{
    authum_char_t buf[authum_logger_line_max];

    if (authum_logger_buffer_log_v(self, level, fmt, ap)) return;

    if (authum_null_eq(fmt) || !authum_logger_is_enabled(self, level))
    {
        return;
    }
    vsnprintf(buf, sizeof buf, fmt, ap);
    authum_logger_channel_write(authum_logger_get_channel(self), level, buf);
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

    if (authum_null_eq(label) || (authum_null_eq(data) && len != 0u) ||
        !authum_logger_is_enabled(self, authum_logger_level_debug))
    {
        return;
    }

    p = (const authum_byte_t *)data;
    authum_logger_debug(self, "%s (%u bytes):", label, (unsigned)len);

    for (i = 0u; i < len; i += 16u)
    {
        int pos = snprintf(line, sizeof line, "  %04x: ", (unsigned)i);
        authum_usize_t j;
        for (j = 0u; j < 16u && (i + j) < len; ++j)
        {
            pos += snprintf(line + pos, sizeof line - (authum_usize_t)pos, "%02x ", p[i + j]);
        }
        authum_logger_debug(self, "%s", line);
    }
}

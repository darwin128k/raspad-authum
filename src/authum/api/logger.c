#include <lh/assert/static.h>
#include <lh/char/map.h>
#include <lh/logger/level.h>
#include <lh/memory/std.h>
#include <lh/str/format/text.h>
#include <authum/logger/buffer.h>


/* Authum's levels are LH's, value for value: the names come from LH. */
lh_assert_static(authum_logger_level_emergency == lh_logger_level_emergency &&
                     authum_logger_level_debug == lh_logger_level_debug &&
                     authum_logger_level_error == lh_logger_level_error &&
                     authum_logger_level_warning == lh_logger_level_warning,
                 "authum and lh logger levels must have the same values");

authum_bool_t
authum_logger_level_parse(const authum_char_t *name, authum_logger_level_t *out)
{
    return !authum_null_eq(out) && lh_logger_level_try_parse(name, out) ? authum_bool_true
                                                                        : authum_bool_false;
}

const authum_char_t *
authum_logger_level_get_name(authum_logger_level_t level)
{
    return lh_logger_level_get_name(level);
}

void
authum_logger_channel_write(const authum_logger_channel_t *channel, authum_logger_level_t level,
                            const authum_char_t *text)
{
    if (!authum_null_eq(channel) && !authum_null_eq(channel->ops) &&
        !authum_null_eq((channel->ops)->write) && !authum_null_eq(text))
    {
        channel->ops->write(channel->owner, level, text);
    }
}

authum_bool_t
authum_logger_channel_is_enabled(const authum_logger_channel_t *channel,
                                 authum_logger_level_t level)
{
    if (authum_null_eq(channel) || authum_null_eq(channel->ops) ||
        authum_null_eq((channel->ops)->write))
    {
        return authum_bool_false;
    }
    /* A host without the filter takes every line. */
    return !authum_null_eq((channel->ops)->is_enabled)
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
    return self->owns_journal ? lh_addr_of(self->journal) : self->channel;
}

void
authum_logger_init(authum_logger_t *self, const authum_host_t *host)
{
    if (!authum_null_eq(self))
    {
        lh_memory_std_set(self, 0, sizeof lh_ptr_deref(self));
        self->channel = authum_host_get_logger(host);
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
        authum_null_eq((module->ops)->open) || authum_null_eq((module->ops)->close) ||
        authum_null_eq(name) || lh_math_eq(name[0], lh_char_map_nul))
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
    lh_memory_std_set(self, 0, sizeof lh_ptr_deref(self));
}

authum_bool_t
authum_logger_create(authum_logger_t *self, const authum_host_t *host,
                     const authum_logger_options_t *options)
{
    if (!self)
    {
        return authum_bool_false;
    }
    authum_logger_init(self, host);
    const authum_logger_channel_t *module = self->channel;
    self->channel = authum_null;
    if (!module || !module->ops || authum_null_eq((module->ops)->construct) ||
        authum_null_eq((module->ops)->close) || !options)
    {
        return authum_bool_false;
    }
    authum_ptr journal = module->ops->construct(module->owner, options);
    if (!journal)
    {
        return authum_bool_false;
    }
    self->journal.ops = module->ops;
    self->journal.owner = journal;
    self->owns_journal = authum_bool_true;
    return authum_bool_true;
}

void
authum_host_log(const authum_host_t *host, authum_logger_level_t level, const authum_char_t *fmt,
                ...)
{
    authum_logger_t log;
    authum_logger_init(lh_addr_of(log), host);
    va_list args;
    va_start(args, fmt);
    authum_logger_log_v(lh_addr_of(log), level, fmt, args);
    va_end(args);
}

authum_bool_t
authum_logger_is_enabled(const authum_logger_t *self, authum_logger_level_t level)
{
    if (authum_logger_buffer_is_active())
    {
        return self && lh_math_le(level, authum_logger_level_debug);
    }
    return authum_logger_channel_is_enabled(authum_logger_get_channel(self), level);
}

void
authum_logger_log_v(authum_logger_t *self, authum_logger_level_t level, const authum_char_t *fmt,
                    va_list ap)
{
    authum_char_t buf[authum_logger_line_max];

    if (authum_logger_buffer_log_v(self, level, fmt, ap))
    {
        return;
    }

    if (authum_null_eq(fmt) || !authum_logger_is_enabled(self, level))
    {
        return;
    }
    authum_cstr_format_v(buf, sizeof buf, fmt, ap);
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

    if (authum_null_eq(label) || (authum_null_eq(data) && lh_math_ne(len, 0u)) ||
        !authum_logger_is_enabled(self, authum_logger_level_debug))
    {
        return;
    }

    p = lh_cast_reinterpret(const authum_byte_t *, data);
    authum_logger_debug(self, "%s (%u bytes):", label, lh_cast_static(authum_u32_t, len));

    for (i = 0u; lh_math_lt(i, len); i = lh_math_add(i, 16u))
    {
        authum_usize_t pos =
            lh_str_ptr_format_cstr(line, sizeof line, "  %04x: ", lh_cast_static(authum_u32_t, i));
        authum_usize_t j;
        for (j = 0u; lh_math_lt(j, 16u) && lh_math_lt((lh_math_add(i, j)), len);
             j = lh_math_add_one(j))
        {
            pos = lh_math_add(
                pos, lh_str_ptr_format_cstr(lh_math_add(line, pos), lh_math_sub(sizeof line, pos),
                                           "%02x ",
                                           lh_cast_static(authum_u32_t, p[lh_math_add(i, j)])));
        }
        authum_logger_debug(self, "%s", line);
    }
}

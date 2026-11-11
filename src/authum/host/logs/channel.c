/**
 * @file channel.c
 * @brief ::authum_logs_channel_t: its fields — see <authum/host/logs/channel.h>.
 */

#include <authum/host/logs/channel.h>
#include <lh/core/memory/std.h>
#include <lh/core/util/ptr.h>

authum_logs_channel_t *
authum_logs_channel_get_next(const authum_logs_channel_t *self)
{
    return self->next;
}

void
authum_logs_channel_set_next(authum_logs_channel_t *self, authum_logs_channel_t *value)
{
    self->next = value;
}

authum_u32_t
authum_logs_channel_get_id(const authum_logs_channel_t *self)
{
    return self->id;
}

void
authum_logs_channel_set_id(authum_logs_channel_t *self, authum_u32_t value)
{
    self->id = value;
}

authum_ptr
authum_logs_channel_get_owner(const authum_logs_channel_t *self)
{
    return self->owner;
}

void
authum_logs_channel_set_owner(authum_logs_channel_t *self, authum_ptr value)
{
    self->owner = value;
}

authum_logs_channel_t *
authum_logs_channel_get_parent(const authum_logs_channel_t *self)
{
    return self->parent;
}

void
authum_logs_channel_set_parent(authum_logs_channel_t *self, authum_logs_channel_t *value)
{
    self->parent = value;
}

authum_usize_t
authum_logs_channel_get_refs(const authum_logs_channel_t *self)
{
    return self->refs;
}

void
authum_logs_channel_set_refs(authum_logs_channel_t *self, authum_usize_t value)
{
    self->refs = value;
}

const authum_char_t *
authum_logs_channel_get_name(const authum_logs_channel_t *self)
{
    return self->name;
}

authum_char_t *
authum_logs_channel_get_name_mut(authum_logs_channel_t *self)
{
    return self->name;
}

const authum_char_t *
authum_logs_channel_get_tag(const authum_logs_channel_t *self)
{
    return self->tag;
}

authum_char_t *
authum_logs_channel_get_tag_mut(authum_logs_channel_t *self)
{
    return self->tag;
}

authum_i32_t
authum_logs_channel_get_level(const authum_logs_channel_t *self)
{
    return self->level;
}

void
authum_logs_channel_set_level(authum_logs_channel_t *self, authum_i32_t value)
{
    self->level = value;
}

authum_i32_t
authum_logs_channel_get_console_level(const authum_logs_channel_t *self)
{
    return self->console_level;
}

void
authum_logs_channel_set_console_level(authum_logs_channel_t *self, authum_i32_t value)
{
    self->console_level = value;
}

authum_i32_t
authum_logs_channel_get_host_level(const authum_logs_channel_t *self)
{
    return self->host_level;
}

void
authum_logs_channel_set_host_level(authum_logs_channel_t *self, authum_i32_t value)
{
    self->host_level = value;
}

long
authum_logs_channel_get_keep_days(const authum_logs_channel_t *self)
{
    return self->keep_days;
}

void
authum_logs_channel_set_keep_days(authum_logs_channel_t *self, long value)
{
    self->keep_days = value;
}

authum_bool_t
authum_logs_channel_is_file_enabled(const authum_logs_channel_t *self)
{
    return self->file_enabled;
}

void
authum_logs_channel_set_file_enabled(authum_logs_channel_t *self, authum_bool_t value)
{
    self->file_enabled = value;
}

authum_logger_format_t
authum_logs_channel_get_format(const authum_logs_channel_t *self)
{
    return self->format;
}

void
authum_logs_channel_set_format(authum_logs_channel_t *self, authum_logger_format_t value)
{
    self->format = value;
}

authum_logger_format_t
authum_logs_channel_get_default_format(const authum_logs_channel_t *self)
{
    return self->default_format;
}

void
authum_logs_channel_set_default_format(authum_logs_channel_t *self, authum_logger_format_t value)
{
    self->default_format = value;
}

const authum_path_t *
authum_logs_channel_get_config(const authum_logs_channel_t *self)
{
    return lh_addr_of(self->config);
}

authum_path_t *
authum_logs_channel_get_config_mut(authum_logs_channel_t *self)
{
    return lh_addr_of(self->config);
}

const authum_path_t *
authum_logs_channel_get_dir(const authum_logs_channel_t *self)
{
    return lh_addr_of(self->dir);
}

authum_path_t *
authum_logs_channel_get_dir_mut(authum_logs_channel_t *self)
{
    return lh_addr_of(self->dir);
}

void
authum_logs_channel_init(authum_logs_channel_t *self)
{
    lh_memory_std_set(self, 0, sizeof lh_ptr_deref(self));
}

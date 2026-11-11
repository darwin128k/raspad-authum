/**
 * @file channel.h
 * @brief ::authum_logs_channel_t — one log channel of the host's registry.
 *
 * The host's own channel (`authum`), one per loaded module, and one per
 * journal a module opens. A channel knows where its daily files go, how
 * verbose each destination is (file, console, the host's copy) and how its
 * lines read; the registry in `host/logs.c` links them and sends them to the
 * log files. Its fields are touched only by the level-1 functions below.
 */

#ifndef AUTHUM_HOST_LOGS_CHANNEL_H
#define AUTHUM_HOST_LOGS_CHANNEL_H

#include <authum/logger/options.h>
#include <authum/path.h>

enum
{
    /** Characters of a channel's folder name, the NUL included. */
    authum_logs_name_max = 64,
    /** Characters of a channel's source mark (`module/journal`), the NUL included. */
    authum_logs_tag_max = 128,
    /** Channel level that lets nothing through. */
    authum_logs_level_off = -1
};

typedef struct authum_logs_channel authum_logs_channel_t;

struct authum_logs_channel
{
    authum_logs_channel_t *next;
    /** The id its records carry in the queue; 0 is the host. */
    authum_u32_t id;
    authum_ptr owner;
    /** The module channel this journal belongs to; null for a module or the host. */
    authum_logs_channel_t *parent;
    /** Opens of a journal by name; it closes when the last one does. */
    authum_usize_t refs;
    /** Folder name: `qproto`, or `bans` under `qproto/` for a journal. */
    authum_char_t name[authum_logs_name_max];
    /** The source mark of the records: `QPROTO`, `QPROTO/bans` for a journal. */
    authum_char_t tag[authum_logs_tag_max];
    /** Most verbose level written to the file, or ::authum_logs_level_off. */
    authum_i32_t level;
    authum_i32_t console_level;
    authum_i32_t host_level;
    long keep_days;
    authum_bool_t file_enabled;
    authum_logger_format_t format;
    authum_logger_format_t default_format;
    authum_path_t config;
    /** Where the daily files go: logs/, modules/<module>/logs/, or a journal's folder in it. */
    authum_path_t dir;
};

AUTHUM_EXTERN_C_BEGIN

/** @brief The next channel of the registry (level 1). */
authum_logs_channel_t *
authum_logs_channel_get_next(const authum_logs_channel_t *self);

/** @brief Store the next channel of the registry (level 1). */
void
authum_logs_channel_set_next(authum_logs_channel_t *self, authum_logs_channel_t *value);

/** @brief The id its records carry in the queue; 0 is the host (level 1). */
authum_u32_t
authum_logs_channel_get_id(const authum_logs_channel_t *self);

/** @brief Store the id (level 1). */
void
authum_logs_channel_set_id(authum_logs_channel_t *self, authum_u32_t value);

/** @brief Who the channel is for: the module, or the journal itself (level 1). */
authum_ptr
authum_logs_channel_get_owner(const authum_logs_channel_t *self);

/** @brief Store the owner (level 1). */
void
authum_logs_channel_set_owner(authum_logs_channel_t *self, authum_ptr value);

/** @brief The module channel of a journal; null for a module or the host (level 1). */
authum_logs_channel_t *
authum_logs_channel_get_parent(const authum_logs_channel_t *self);

/** @brief Store the parent (level 1). */
void
authum_logs_channel_set_parent(authum_logs_channel_t *self, authum_logs_channel_t *value);

/** @brief Opens of a journal by name (level 1). */
authum_usize_t
authum_logs_channel_get_refs(const authum_logs_channel_t *self);

/** @brief Store the opens (level 1). */
void
authum_logs_channel_set_refs(authum_logs_channel_t *self, authum_usize_t value);

/** @brief The folder name, NUL-terminated (level 1). */
const authum_char_t *
authum_logs_channel_get_name(const authum_logs_channel_t *self);

/** @brief The folder name, ::authum_logs_name_max characters to write (level 1). */
authum_char_t *
authum_logs_channel_get_name_mut(authum_logs_channel_t *self);

/** @brief The source mark, NUL-terminated (level 1). */
const authum_char_t *
authum_logs_channel_get_tag(const authum_logs_channel_t *self);

/** @brief The source mark, ::authum_logs_tag_max characters to write (level 1). */
authum_char_t *
authum_logs_channel_get_tag_mut(authum_logs_channel_t *self);

/** @brief The most verbose level written to the file, or ::authum_logs_level_off (level 1). */
authum_i32_t
authum_logs_channel_get_level(const authum_logs_channel_t *self);

/** @brief Store the file level (level 1). */
void
authum_logs_channel_set_level(authum_logs_channel_t *self, authum_i32_t value);

/** @brief The most verbose level mirrored to the console (level 1). */
authum_i32_t
authum_logs_channel_get_console_level(const authum_logs_channel_t *self);

/** @brief Store the console level (level 1). */
void
authum_logs_channel_set_console_level(authum_logs_channel_t *self, authum_i32_t value);

/** @brief The most verbose level copied to the host's channel (level 1). */
authum_i32_t
authum_logs_channel_get_host_level(const authum_logs_channel_t *self);

/** @brief Store the host level (level 1). */
void
authum_logs_channel_set_host_level(authum_logs_channel_t *self, authum_i32_t value);

/** @brief Days the daily files are kept; 0 keeps them all (level 1). */
long
authum_logs_channel_get_keep_days(const authum_logs_channel_t *self);

/** @brief Store the days kept (level 1). */
void
authum_logs_channel_set_keep_days(authum_logs_channel_t *self, long value);

/** @brief Whether the channel writes its own file (level 1). */
authum_bool_t
authum_logs_channel_is_file_enabled(const authum_logs_channel_t *self);

/** @brief Store whether the channel writes its own file (level 1). */
void
authum_logs_channel_set_file_enabled(authum_logs_channel_t *self, authum_bool_t value);

/** @brief How its lines read (level 1). */
authum_logger_format_t
authum_logs_channel_get_format(const authum_logs_channel_t *self);

/** @brief Store how its lines read (level 1). */
void
authum_logs_channel_set_format(authum_logs_channel_t *self, authum_logger_format_t value);

/** @brief The format the module asked for, before the config (level 1). */
authum_logger_format_t
authum_logs_channel_get_default_format(const authum_logs_channel_t *self);

/** @brief Store the format the module asked for (level 1). */
void
authum_logs_channel_set_default_format(authum_logs_channel_t *self, authum_logger_format_t value);

/** @brief The module's own config file (level 1). */
const authum_path_t *
authum_logs_channel_get_config(const authum_logs_channel_t *self);

/** @brief The module's own config file, for change (level 1). */
authum_path_t *
authum_logs_channel_get_config_mut(authum_logs_channel_t *self);

/** @brief Where the daily files go (level 1). */
const authum_path_t *
authum_logs_channel_get_dir(const authum_logs_channel_t *self);

/** @brief Where the daily files go, for change (level 1). */
authum_path_t *
authum_logs_channel_get_dir_mut(authum_logs_channel_t *self);

/**
 * @brief Every field empty: no links, no name, levels 0, no files (level 2).
 *        The registry fills it.
 */
void
authum_logs_channel_init(authum_logs_channel_t *self);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_HOST_LOGS_CHANNEL_H */

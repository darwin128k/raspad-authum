/**
 * @file logs.c
 * @brief Log channels: one daily file per module under `logs/<name>/`,
 *        levels from `authum.cfg` — see <authum/logger/host.h>.
 */

#include <authum/logger/host.h>

#include <authum/cfg.h>
#include <authum/cstr.h>
#include <authum/logger.h>
#include <authum/null.h>
#include <authum/path.h>

#include <lh/runtime/allocator.h>

#include <stdio.h>
#include <string.h>

enum
{
    authum_logs_name_max = 64,
    /** Channel level that lets nothing through. */
    authum_logs_level_off = -1
};

typedef struct authum_logs_channel authum_logs_channel_t;

struct authum_logs_channel
{
    authum_logs_channel_t *next;
    authum_ptr owner;
    authum_char_t name[authum_logs_name_max];
    /** Most verbose level written to the file, or ::authum_logs_level_off. */
    int level;
    FILE *file;
    /** YYYYMMDD of @c file; a different day opens a new one. */
    long day;
};

authum_path_t authum_logs_root;
authum_logs_mirror_fn authum_logs_mirror;
authum_logs_clock_fn authum_logs_clock;
int authum_logs_default_level = authum_logger_level_info;
int authum_logs_console_level = authum_logger_level_warning;
authum_logs_channel_t authum_logs_host;
authum_logs_channel_t *authum_logs_head;
authum_bool_t authum_logs_ready;

/** `off` / `none` → off; a level name → that level; anything else → @p fallback. */
int
authum_logs_parse_level(const authum_char_t *value, int fallback)
{
    authum_logger_level_t level;

    if (authum_null_eq(value))
    {
        return fallback;
    }
    if (authum_cstr_eq(value, "off", authum_bool_true) ||
        authum_cstr_eq(value, "none", authum_bool_true))
    {
        return authum_logs_level_off;
    }
    return authum_logger_level_parse(value, &level) ? (int)level : fallback;
}

/** @p key from `<root>/authum.cfg` as a level; @p fallback when absent or unreadable. */
int
authum_logs_read_level(const authum_char_t *key, int fallback)
{
    authum_path_t path;
    authum_cfg_t cfg;
    int level;

    if (!authum_path_join(&path, &authum_logs_root, "authum.cfg") || !authum_cfg_open(&cfg, &path))
    {
        return fallback;
    }
    level = authum_logs_parse_level(authum_cfg_get_value(&cfg, key), fallback);
    authum_cfg_close(&cfg);
    return level;
}

/** Copy @p name keeping it a safe directory name. */
void
authum_logs_set_name(authum_logs_channel_t *channel, const authum_char_t *name)
{
    authum_usize_t i;

    for (i = 0u; name[i] != '\0' && i < sizeof channel->name - 1u; ++i)
    {
        authum_char_t c = name[i];
        channel->name[i] = ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                            (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.')
                               ? c
                               : '_';
    }
    channel->name[i] = '\0';
    if (i == 0u || strcmp(channel->name, ".") == 0 || strcmp(channel->name, "..") == 0)
    {
        authum_cstr_copy(channel->name, sizeof channel->name, "module");
    }
}

/** The channel's file for @p tm's day, opening (and creating folders) as needed. */
FILE *
authum_logs_file(authum_logs_channel_t *channel, const struct tm *tm)
{
    authum_path_t logs;
    authum_path_t dir;
    authum_path_t path;
    authum_char_t leaf[16];
    long day = (long)(tm->tm_year + 1900) * 10000L + (long)(tm->tm_mon + 1) * 100L + tm->tm_mday;

    if (!authum_null_eq(channel->file) && channel->day == day)
    {
        return channel->file;
    }
    if (!authum_null_eq(channel->file))
    {
        fclose(channel->file);
        channel->file = authum_null;
    }
    snprintf(leaf, sizeof leaf, "%04d-%02d-%02d.log", tm->tm_year + 1900, tm->tm_mon + 1,
             tm->tm_mday);
    if (!authum_path_join(&logs, &authum_logs_root, "logs") || !authum_path_ensure_dir(&logs) ||
        !authum_path_join(&dir, &logs, channel->name) || !authum_path_ensure_dir(&dir) ||
        !authum_path_join(&path, &dir, leaf))
    {
        return authum_null;
    }
    channel->file = fopen(authum_path_as_cstr(&path), "a");
    if (authum_null_eq(channel->file))
    {
        return authum_null;
    }
    setvbuf(channel->file, authum_null, _IONBF, 0);
    channel->day = day;
    return channel->file;
}

/** One line into @p channel's file; @p from tags a line copied from a module. */
void
authum_logs_emit(authum_logs_channel_t *channel, authum_logger_level_t level,
                 const authum_char_t *text, const authum_char_t *from)
{
    time_t now = authum_logs_clock != 0 ? authum_logs_clock() : time(authum_null);
    struct tm *tm = localtime(&now);
    FILE *file;

    if (authum_null_eq(tm))
    {
        return;
    }
    file = authum_logs_file(channel, tm);
    if (authum_null_eq(file))
    {
        return;
    }
    fprintf(file, "[%02d:%02d:%02d] [%s] ", tm->tm_hour, tm->tm_min, tm->tm_sec,
            authum_logger_level_name(level));
    if (!authum_null_eq(from))
    {
        fprintf(file, "[%s] ", from);
    }
    fprintf(file, "%s\n", text);
}

authum_logs_channel_t *
authum_logs_find(authum_ptr owner)
{
    authum_logs_channel_t *channel;

    if (authum_null_eq(owner))
    {
        return &authum_logs_host;
    }
    for (channel = authum_logs_head; !authum_null_eq(channel); channel = channel->next)
    {
        if (channel->owner == owner)
        {
            return channel;
        }
    }
    return authum_null;
}

/** A module's warning or worse also lands in the host channel. */
authum_bool_t
authum_logs_copies_to_host(const authum_logs_channel_t *channel, authum_logger_level_t level)
{
    return (channel != &authum_logs_host && level <= authum_logger_level_warning &&
            (int)level <= authum_logs_host.level)
               ? authum_bool_true
               : authum_bool_false;
}

authum_bool_t
authum_logs_mirrors(authum_logger_level_t level)
{
    return (authum_logs_mirror != 0 && (int)level <= authum_logs_console_level) ? authum_bool_true
                                                                                : authum_bool_false;
}

authum_bool_t
authum_logs_is_enabled(authum_ptr owner, authum_logger_level_t level)
{
    authum_logs_channel_t *channel;

    if (!authum_logs_ready)
    {
        return authum_bool_true; /* stderr takes everything */
    }
    channel = authum_logs_find(owner);
    if (authum_null_eq(channel))
    {
        return authum_bool_false;
    }
    return ((int)level <= channel->level || authum_logs_copies_to_host(channel, level) ||
            authum_logs_mirrors(level))
               ? authum_bool_true
               : authum_bool_false;
}

void
authum_logs_write(authum_ptr owner, authum_logger_level_t level, const authum_char_t *text)
{
    authum_logs_channel_t *channel;

    if (authum_null_eq(text))
    {
        return;
    }
    if (!authum_logs_ready)
    {
        fprintf(stderr, "[authum] [%s] %s\n", authum_logger_level_name(level), text);
        return;
    }
    channel = authum_logs_find(owner);
    if (authum_null_eq(channel))
    {
        return;
    }
    if ((int)level <= channel->level)
    {
        authum_logs_emit(channel, level, text, authum_null);
    }
    if (authum_logs_copies_to_host(channel, level))
    {
        authum_logs_emit(&authum_logs_host, level, text, channel->name);
    }
    if (authum_logs_mirrors(level))
    {
        authum_logs_mirror(level, channel->name, text);
    }
}

authum_bool_t
authum_logs_open(authum_ptr owner, const authum_char_t *name)
{
    authum_logs_channel_t *channel;
    authum_char_t key[authum_logs_name_max + 16];

    if (authum_null_eq(owner) || authum_null_eq(name))
    {
        return authum_bool_false;
    }
    if (!authum_null_eq(authum_logs_find(owner)))
    {
        return authum_bool_true;
    }
    channel = (authum_logs_channel_t *)lh_runtime_allocator_alloc(sizeof *channel);
    if (authum_null_eq(channel))
    {
        return authum_bool_false;
    }
    memset(channel, 0, sizeof *channel);
    channel->owner = owner;
    authum_logs_set_name(channel, name);
    snprintf(key, sizeof key, "%s.log_level", channel->name);
    channel->level = authum_logs_read_level(key, authum_logs_default_level);
    channel->next = authum_logs_head;
    authum_logs_head = channel;
    return authum_bool_true;
}

void
authum_logs_close(authum_ptr owner)
{
    authum_logs_channel_t **link;
    authum_logs_channel_t *channel;

    for (link = &authum_logs_head; !authum_null_eq(*link); link = &(*link)->next)
    {
        channel = *link;
        if (channel->owner == owner)
        {
            *link = channel->next;
            if (!authum_null_eq(channel->file))
            {
                fclose(channel->file);
            }
            lh_runtime_allocator_free(channel);
            return;
        }
    }
}

authum_bool_t
authum_logs_init(const authum_char_t *root, authum_logs_mirror_fn mirror)
{
    authum_logs_deinit();
    if (!authum_path_set_cstr(&authum_logs_root, root))
    {
        return authum_bool_false;
    }
    authum_logs_mirror = mirror;
    authum_logs_default_level = authum_logs_read_level("log_level", authum_logger_level_info);
    authum_logs_console_level = authum_logs_read_level("log_console", authum_logger_level_warning);
    memset(&authum_logs_host, 0, sizeof authum_logs_host);
    authum_logs_set_name(&authum_logs_host, "authum");
    authum_logs_host.level = authum_logs_read_level("authum.log_level", authum_logs_default_level);
    authum_logs_ready = authum_bool_true;
    return authum_bool_true;
}

void
authum_logs_deinit(void)
{
    while (!authum_null_eq(authum_logs_head))
    {
        authum_logs_close(authum_logs_head->owner);
    }
    if (!authum_null_eq(authum_logs_host.file))
    {
        fclose(authum_logs_host.file);
        authum_logs_host.file = authum_null;
    }
    authum_logs_mirror = 0;
    authum_logs_ready = authum_bool_false;
}

void
authum_logs_set_clock(authum_logs_clock_fn clock)
{
    authum_logs_clock = clock;
}

const authum_logger_ops_t authum_logs_ops_table = {
    .size = sizeof(authum_logger_ops_t),
    .write = authum_logs_write,
    .is_enabled = authum_logs_is_enabled,
};

const authum_logger_ops_t *
authum_logs_ops(void)
{
    return &authum_logs_ops_table;
}

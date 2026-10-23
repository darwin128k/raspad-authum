/**
 * @file logs.c
 * @brief Log channels: one daily file per module (and per named journal)
 *        under `logs/`, levels from `authum.cfg` — see <authum/logger/host.h>.
 */

#include <authum/logger/host.h>

#include <authum/cfg.h>
#include <authum/cstr.h>
#include <authum/logger.h>
#include <authum/null.h>
#include <authum/path.h>

#include <lh/fs/kind.h>
#include <lh/fs/path.h>
#include <lh/fs/path/style.h>
#include <lh/os/fs/dir.h>
#include <lh/runtime/allocator.h>
#include <lh/str/view.h>
#include <lh/util/addr.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum
{
    authum_logs_name_max = 64,
    /** Channel level that lets nothing through. */
    authum_logs_level_off = -1,
    /** Bytes buffered per file before stdio writes them on its own. */
    authum_logs_buffer_size = 8192
};

typedef struct authum_logs_channel authum_logs_channel_t;

struct authum_logs_channel
{
    authum_logs_channel_t *next;
    authum_ptr owner;
    /** The module channel this journal belongs to; null for a module or the host. */
    authum_logs_channel_t *parent;
    /** Opens of a journal by name; it closes when the last one does. */
    authum_usize_t refs;
    /** Folder name: `qproto`, or `bans` under `qproto/` for a journal. */
    authum_char_t name[authum_logs_name_max];
    /** How lines are tagged elsewhere: `qproto`, `qproto/bans`. */
    authum_char_t tag[authum_logs_name_max * 2];
    /** Most verbose level written to the file, or ::authum_logs_level_off. */
    int level;
    FILE *file;
    /** YYYYMMDD of @c file; a different day opens a new one. */
    long day;
    /** Written since the last flush. */
    authum_bool_t dirty;
};

authum_path_t authum_logs_root;
authum_logs_mirror_fn authum_logs_mirror;
authum_logs_clock_fn authum_logs_clock;
int authum_logs_default_level = authum_logger_level_info;
int authum_logs_console_level = authum_logger_level_warning;
/** Files older than this many days are deleted; 0 keeps everything. */
long authum_logs_keep_days;
authum_logs_channel_t authum_logs_host;
authum_logs_channel_t *authum_logs_head;
authum_bool_t authum_logs_ready;

/* --- config --------------------------------------------------------------- */

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

authum_bool_t
authum_logs_cfg_open(authum_cfg_t *cfg)
{
    authum_path_t path;

    return (authum_path_join(&path, &authum_logs_root, "configs/authum.cfg") &&
            authum_cfg_open(cfg, &path))
               ? authum_bool_true
               : authum_bool_false;
}

/** The level a channel gets: `<tag with dots>.log_level`, else its parent's / the default. */
void
authum_logs_read_channel_level(authum_logs_channel_t *channel, authum_cfg_t *cfg)
{
    authum_char_t key[sizeof channel->tag + 16];
    authum_usize_t i;
    int fallback =
        !authum_null_eq(channel->parent) ? channel->parent->level : authum_logs_default_level;

    snprintf(key, sizeof key, "%s.log_level", channel->tag);
    for (i = 0u; key[i] != '\0'; ++i)
    {
        key[i] = key[i] == '/' ? '.' : key[i];
    }
    channel->level = !authum_null_eq(cfg)
                         ? authum_logs_parse_level(authum_cfg_get_value(cfg, key), fallback)
                         : fallback;
}

/** Global keys, then every channel's own level (modules before their journals). */
void
authum_logs_read_cfg(void)
{
    authum_cfg_t cfg;
    authum_cfg_t *open = authum_logs_cfg_open(&cfg) ? &cfg : authum_null;
    authum_logs_channel_t *channel;
    authum_i32_t keep;
    int pass;

    authum_logs_default_level = authum_logger_level_info;
    authum_logs_console_level = authum_logger_level_warning;
    authum_logs_keep_days = 0;
    if (!authum_null_eq(open))
    {
        authum_logs_default_level = authum_logs_parse_level(authum_cfg_get_value(open, "log_level"),
                                                            authum_logs_default_level);
        authum_logs_console_level = authum_logs_parse_level(
            authum_cfg_get_value(open, "log_console"), authum_logs_console_level);
        keep = authum_cfg_get_i32(open, "log_keep_days", 0);
        authum_logs_keep_days = keep > 0 ? (long)keep : 0;
    }
    authum_logs_read_channel_level(&authum_logs_host, open);
    for (pass = 0; pass < 2; ++pass)
    {
        for (channel = authum_logs_head; !authum_null_eq(channel); channel = channel->next)
        {
            if ((pass == 0) == authum_null_eq(channel->parent))
            {
                authum_logs_read_channel_level(channel, open);
            }
        }
    }
    if (!authum_null_eq(open))
    {
        authum_cfg_close(open);
    }
}

/* --- files ---------------------------------------------------------------- */

/** Copy @p name keeping it a safe directory name. */
void
authum_logs_set_name(authum_char_t *out, authum_usize_t out_size, const authum_char_t *name)
{
    authum_usize_t i;

    for (i = 0u; name[i] != '\0' && i < out_size - 1u; ++i)
    {
        authum_char_t c = name[i];
        out[i] = ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                  c == '_' || c == '-' || c == '.')
                     ? c
                     : '_';
    }
    out[i] = '\0';
    if (i == 0u || strcmp(out, ".") == 0 || strcmp(out, "..") == 0)
    {
        authum_cstr_copy(out, out_size, "module");
    }
}

/** `logs/<module>[/<journal>]`, created on the way. */
authum_bool_t
authum_logs_channel_dir(const authum_logs_channel_t *channel, authum_path_t *out)
{
    authum_path_t logs;
    authum_path_t module;

    if (!authum_path_join(&logs, &authum_logs_root, "logs") || !authum_path_ensure_dir(&logs))
    {
        return authum_bool_false;
    }
    if (authum_null_eq(channel->parent))
    {
        return (authum_path_join(out, &logs, channel->name) && authum_path_ensure_dir(out))
                   ? authum_bool_true
                   : authum_bool_false;
    }
    return (authum_path_join(&module, &logs, channel->parent->name) &&
            authum_path_ensure_dir(&module) && authum_path_join(out, &module, channel->name) &&
            authum_path_ensure_dir(out))
               ? authum_bool_true
               : authum_bool_false;
}

/** YYYYMMDD from a `YYYY-MM-DD.log` file name, or 0. */
long
authum_logs_file_day(const authum_char_t *name, authum_usize_t size)
{
    authum_usize_t i;

    if (size != 14u || name[4] != '-' || name[7] != '-' || strncmp(name + 10, ".log", 4) != 0)
    {
        return 0;
    }
    for (i = 0u; i < 10u; ++i)
    {
        if (i != 4u && i != 7u && (name[i] < '0' || name[i] > '9'))
        {
            return 0;
        }
    }
    return atol(name) * 10000L + atol(name + 5) * 100L + atol(name + 8);
}

/** Delete this folder's daily files older than ::authum_logs_keep_days. */
void
authum_logs_prune(const authum_path_t *dir, const struct tm *today)
{
    struct tm cutoff_tm = *today;
    long cutoff;
    lh_fs_path_t lh_dir;
    lh_str_view_t text;
    lh_os_fs_dir_t listing;
    lh_os_fs_dir_entry_kind_t kind;
    lh_str_view_t entry;
    authum_char_t leaf[32];
    authum_path_t path;
    lh_ssize_t n;

    if (authum_logs_keep_days <= 0)
    {
        return;
    }
    cutoff_tm.tm_mday -= (int)authum_logs_keep_days;
    cutoff_tm.tm_isdst = -1;
    if (mktime(&cutoff_tm) == (time_t)-1)
    {
        return;
    }
    cutoff = (long)(cutoff_tm.tm_year + 1900) * 10000L + (long)(cutoff_tm.tm_mon + 1) * 100L +
             cutoff_tm.tm_mday;

    lh_fs_path_init(lh_addr_of(lh_dir));
    lh_str_view_init(lh_addr_of(text), authum_path_as_cstr(dir));
    lh_fs_path_set(lh_addr_of(lh_dir), text, lh_fs_path_style_posix);
    lh_os_fs_dir_init(lh_addr_of(listing));
    if (lh_os_fs_dir_open(lh_addr_of(listing), lh_addr_of(lh_dir)))
    {
        while ((n = lh_os_fs_dir_read(lh_addr_of(listing), lh_addr_of(entry), lh_addr_of(kind))) >
               0)
        {
            long day;

            if (kind == lh_fs_kind_dir || (authum_usize_t)n >= sizeof leaf)
            {
                continue;
            }
            memcpy(leaf, lh_str_view_get_data(lh_addr_of(entry)), (size_t)n);
            leaf[n] = '\0';
            day = authum_logs_file_day(leaf, (authum_usize_t)n);
            if (day != 0 && day < cutoff && authum_path_join(&path, dir, leaf))
            {
                remove(authum_path_as_cstr(&path));
            }
        }
        lh_os_fs_dir_close(lh_addr_of(listing));
    }
    lh_fs_path_deinit(lh_addr_of(lh_dir));
}

/** The channel's file for @p tm's day, opening (and pruning) on a new day. */
FILE *
authum_logs_file(authum_logs_channel_t *channel, const struct tm *tm)
{
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
    if (!authum_logs_channel_dir(channel, &dir) || !authum_path_join(&path, &dir, leaf))
    {
        return authum_null;
    }
    authum_logs_prune(&dir, tm);
    channel->file = fopen(authum_path_as_cstr(&path), "a");
    if (authum_null_eq(channel->file))
    {
        return authum_null;
    }
    setvbuf(channel->file, authum_null, _IOFBF, authum_logs_buffer_size);
    channel->day = day;
    channel->dirty = authum_bool_false;
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
    /* An error may come right before a crash: put it on disk now. */
    if (level <= authum_logger_level_error)
    {
        fflush(file);
        channel->dirty = authum_bool_false;
    }
    else
    {
        channel->dirty = authum_bool_true;
    }
}

void
authum_logs_flush_channel(authum_logs_channel_t *channel)
{
    if (channel->dirty && !authum_null_eq(channel->file))
    {
        fflush(channel->file);
    }
    channel->dirty = authum_bool_false;
}

void
authum_logs_close_file(authum_logs_channel_t *channel)
{
    if (!authum_null_eq(channel->file))
    {
        fclose(channel->file);
        channel->file = authum_null;
    }
    channel->dirty = authum_bool_false;
}

/* --- channels ------------------------------------------------------------- */

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

/** A module's (or journal's) warning or worse also lands in the host channel. */
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
        authum_logs_emit(&authum_logs_host, level, text, channel->tag);
    }
    if (authum_logs_mirrors(level))
    {
        authum_logs_mirror(level, channel->tag, text);
    }
}

authum_logs_channel_t *
authum_logs_add(authum_ptr owner, authum_logs_channel_t *parent, const authum_char_t *name)
{
    authum_logs_channel_t *channel;
    authum_cfg_t cfg;

    channel = (authum_logs_channel_t *)lh_runtime_allocator_alloc(sizeof *channel);
    if (authum_null_eq(channel))
    {
        return authum_null;
    }
    memset(channel, 0, sizeof *channel);
    channel->owner = authum_null_eq(owner) ? (authum_ptr)channel : owner;
    channel->parent = parent;
    channel->refs = 1u;
    authum_logs_set_name(channel->name, sizeof channel->name, name);
    if (authum_null_eq(parent))
    {
        authum_cstr_copy(channel->tag, sizeof channel->tag, channel->name);
    }
    else
    {
        snprintf(channel->tag, sizeof channel->tag, "%s/%s", parent->name, channel->name);
    }
    if (authum_logs_cfg_open(&cfg))
    {
        authum_logs_read_channel_level(channel, &cfg);
        authum_cfg_close(&cfg);
    }
    else
    {
        authum_logs_read_channel_level(channel, authum_null);
    }
    channel->next = authum_logs_head;
    authum_logs_head = channel;
    return channel;
}

void
authum_logs_free(authum_logs_channel_t *channel)
{
    authum_logs_channel_t **link;

    for (link = &authum_logs_head; !authum_null_eq(*link); link = &(*link)->next)
    {
        if (*link == channel)
        {
            *link = channel->next;
            authum_logs_close_file(channel);
            lh_runtime_allocator_free(channel);
            return;
        }
    }
}

authum_bool_t
authum_logs_open(authum_ptr owner, const authum_char_t *name)
{
    if (authum_null_eq(owner) || authum_null_eq(name))
    {
        return authum_bool_false;
    }
    if (!authum_null_eq(authum_logs_find(owner)))
    {
        return authum_bool_true;
    }
    return !authum_null_eq(authum_logs_add(owner, authum_null, name)) ? authum_bool_true
                                                                      : authum_bool_false;
}

void
authum_logs_close(authum_ptr owner)
{
    authum_logs_channel_t *channel = authum_null_eq(owner) ? authum_null : authum_logs_find(owner);
    authum_logs_channel_t *it;

    if (authum_null_eq(channel))
    {
        return;
    }
    /* Journals the module did not close go with it. */
    for (it = authum_logs_head; !authum_null_eq(it);)
    {
        authum_logs_channel_t *next = it->next;
        if (it->parent == channel)
        {
            authum_logs_free(it);
        }
        it = next;
    }
    authum_logs_free(channel);
}

authum_ptr
authum_logs_open_journal(authum_ptr owner, const authum_char_t *name)
{
    authum_logs_channel_t *module = authum_null_eq(owner) ? authum_null : authum_logs_find(owner);
    authum_logs_channel_t *channel;
    authum_char_t clean[authum_logs_name_max];

    if (authum_null_eq(module) || !authum_null_eq(module->parent) || authum_null_eq(name) ||
        name[0] == '\0')
    {
        return authum_null;
    }
    authum_logs_set_name(clean, sizeof clean, name);
    for (channel = authum_logs_head; !authum_null_eq(channel); channel = channel->next)
    {
        if (channel->parent == module && strcmp(channel->name, clean) == 0)
        {
            ++channel->refs;
            return channel->owner;
        }
    }
    channel = authum_logs_add(authum_null, module, clean);
    return !authum_null_eq(channel) ? channel->owner : authum_null;
}

void
authum_logs_close_journal(authum_ptr journal)
{
    authum_logs_channel_t *channel =
        authum_null_eq(journal) ? authum_null : authum_logs_find(journal);

    if (authum_null_eq(channel) || authum_null_eq(channel->parent))
    {
        return;
    }
    if (--channel->refs == 0u)
    {
        authum_logs_free(channel);
    }
}

/* --- lifetime ------------------------------------------------------------- */

authum_bool_t
authum_logs_init(const authum_char_t *root, authum_logs_mirror_fn mirror)
{
    authum_logs_deinit();
    if (!authum_path_set_cstr(&authum_logs_root, root))
    {
        return authum_bool_false;
    }
    authum_logs_mirror = mirror;
    memset(&authum_logs_host, 0, sizeof authum_logs_host);
    authum_logs_set_name(authum_logs_host.name, sizeof authum_logs_host.name, "authum");
    authum_cstr_copy(authum_logs_host.tag, sizeof authum_logs_host.tag, "authum");
    authum_logs_read_cfg();
    authum_logs_ready = authum_bool_true;
    return authum_bool_true;
}

void
authum_logs_reload(void)
{
    if (authum_logs_ready)
    {
        authum_logs_read_cfg();
    }
}

void
authum_logs_flush(void)
{
    authum_logs_channel_t *channel;

    authum_logs_flush_channel(&authum_logs_host);
    for (channel = authum_logs_head; !authum_null_eq(channel); channel = channel->next)
    {
        authum_logs_flush_channel(channel);
    }
}

void
authum_logs_deinit(void)
{
    while (!authum_null_eq(authum_logs_head))
    {
        authum_logs_free(authum_logs_head);
    }
    authum_logs_close_file(&authum_logs_host);
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
    .open = authum_logs_open_journal,
    .close = authum_logs_close_journal,
};

const authum_logger_ops_t *
authum_logs_ops(void)
{
    return &authum_logs_ops_table;
}

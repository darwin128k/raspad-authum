#include "files.h"
#include <authum/cstr.h>
#include <authum/null.h>
#include <authum/logger.h>
#include <lh/fs/kind.h>
#include <lh/fs/path.h>
#include <lh/fs/path/style.h>
#include <lh/os/fs/dir.h>
#include <lh/str/view.h>
#include <lh/util/addr.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

enum
{
    authum_logs_buffer_size = 8192,
    authum_logs_file_capacity = 32
};

typedef struct authum_logs_file
{
    authum_path_t dir;
    FILE *file;
    long day;
    long keep_days;
    authum_bool_t dirty;
    authum_usize_t used;
} authum_logs_file_t;

static authum_logs_file_t files[authum_logs_file_capacity];
static authum_usize_t sequence;
static authum_path_t report_dir;
static authum_bool_t reported_error;

/* Report the first disk failure to stderr without recursing into the logger.
 * All file operations and this diagnostic stay on the writer thread. */
static void
file_error(const authum_path_t *path)
{
    if (!reported_error)
    {
        int saved_error = errno;
        fprintf(stderr, "authum logger: file I/O failed, path='%s': %s\n",
                authum_path_as_cstr(path), strerror(saved_error));
        reported_error = authum_bool_true;
    }
}

static authum_logs_file_t *
authum_logs_files_slot(const authum_path_t *dir)
{
    authum_logs_file_t *oldest = &files[0];
    for (authum_usize_t i = 0; i < authum_logs_file_capacity; ++i)
    {
        authum_logs_file_t *slot = &files[i];
        if (strcmp(authum_path_as_cstr(&slot->dir), authum_path_as_cstr(dir)) == 0)
        {
            slot->used = ++sequence;
            return slot;
        }
        if (slot->used < oldest->used)
        {
            oldest = slot;
        }
    }
    if (oldest->file)
    {
        if (fclose(oldest->file) != 0)
        {
            file_error(&oldest->dir);
        }
    }
    memset(oldest, 0, sizeof *oldest);
    oldest->dir = *dir;
    oldest->used = ++sequence;
    return oldest;
}

/** Create @p dir and up to @p depth missing parents above it. */
static authum_bool_t
authum_logs_ensure_tree(const authum_path_t *dir, int depth)
{
    authum_path_t parent;

    if (authum_path_ensure_dir(dir))
    {
        return authum_bool_true;
    }
    return (depth > 0 && authum_path_parent(&parent, dir) &&
            authum_logs_ensure_tree(&parent, depth - 1) && authum_path_ensure_dir(dir))
               ? authum_bool_true
               : authum_bool_false;
}

static authum_bool_t
authum_logs_channel_dir(const authum_logs_file_t *channel, authum_path_t *out)
{
    /* logs/, modules/<module>/logs/ or a journal's folder in it: create what is missing. */
    return (authum_path_assign(out, &channel->dir) && authum_logs_ensure_tree(out, 3))
               ? authum_bool_true
               : authum_bool_false;
}

/** YYYYMMDD from a `YYYY-MM-DD.log` file name, or 0. */
static long
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

/** Delete this folder's daily files older than ::keep_days. */
static void
authum_logs_prune(const authum_path_t *dir, const struct tm *today, long keep_days)
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

    if (keep_days <= 0)
    {
        return;
    }
    cutoff_tm.tm_mday -= (int)keep_days;
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
static FILE *
authum_logs_file(authum_logs_file_t *channel, const struct tm *tm)
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
        if (fclose(channel->file) != 0)
        {
            file_error(&channel->dir);
        }
        channel->file = authum_null;
    }
    if (!strftime(leaf, sizeof leaf, "%Y-%m-%d.log", tm))
    {
        return authum_null;
    }
    if (!authum_logs_channel_dir(channel, &dir) || !authum_path_join(&path, &dir, leaf))
    {
        file_error(&channel->dir);
        return authum_null;
    }
    authum_logs_prune(&dir, tm, channel->keep_days);
    channel->file = fopen(authum_path_as_cstr(&path), "a");
    if (authum_null_eq(channel->file))
    {
        file_error(&path);
        return authum_null;
    }
    setvbuf(channel->file, authum_null, _IOFBF, authum_logs_buffer_size);
    channel->day = day;
    channel->dirty = authum_bool_false;
    return channel->file;
}

/** All calls below run on the writer thread. */
void
authum_logs_files_emit(const authum_logs_record_t *record)
{
    authum_logs_file_t *channel = authum_logs_files_slot(&record->dir);
    authum_logger_level_t level = record->level;
    const authum_char_t *text = record->text;
    const authum_char_t *from = record->tag[0] ? record->tag : authum_null;
    struct tm calendar;
    struct tm *tm = &calendar;
    channel->keep_days = record->keep_days;
#if defined(_WIN32)
    if (localtime_s(tm, &record->time) != 0)
#else
    if (!localtime_r(&record->time, tm))
#endif
    {
        return;
    }
    FILE *file;

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
    if (fprintf(file, "%s\n", text) < 0)
    {
        file_error(&channel->dir);
    }
    /* An error may come right before a crash: put it on disk now. */
    if (level <= authum_logger_level_error)
    {
        if (fflush(file) != 0)
        {
            file_error(&channel->dir);
        }
        channel->dirty = authum_bool_false;
    }
    else
    {
        channel->dirty = authum_bool_true;
    }
}

static void
authum_logs_flush_channel(authum_logs_file_t *channel)
{
    if (channel->dirty && !authum_null_eq(channel->file))
    {
        if (fflush(channel->file) != 0)
        {
            file_error(&channel->dir);
        }
    }
    channel->dirty = authum_bool_false;
}

static void
authum_logs_close_file(authum_logs_file_t *channel)
{
    if (!authum_null_eq(channel->file))
    {
        if (fclose(channel->file) != 0)
        {
            file_error(&channel->dir);
        }
        channel->file = authum_null;
    }
    channel->dirty = authum_bool_false;
}

void
authum_logs_files_flush(void)
{
    for (authum_usize_t i = 0; i < authum_logs_file_capacity; ++i)
    {
        authum_logs_flush_channel(&files[i]);
    }
}

void
authum_logs_files_close(void)
{
    for (authum_usize_t i = 0; i < authum_logs_file_capacity; ++i)
    {
        authum_logs_close_file(&files[i]);
    }
    memset(files, 0, sizeof files);
    sequence = 0;
    reported_error = authum_bool_false;
}

void
authum_logs_files_set_root(const authum_path_t *dir)
{
    report_dir = *dir;
}

void
authum_logs_files_dropped(authum_u32_t count)
{
    authum_logs_record_t record = {0};
    record.dir = report_dir;
    record.level = authum_logger_level_warning;
    record.time = time(NULL);
    authum_cstr_copy(record.tag, sizeof record.tag, "logger");
    snprintf(record.text, sizeof record.text,
             "queue full: dropped %u file records; game thread did not wait", (unsigned)count);
    authum_logs_files_emit(&record);
}

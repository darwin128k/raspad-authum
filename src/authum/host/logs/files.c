#include <authum/host/logs/files.h>
#include <lh/char/digit.h>
#include <authum/memory.h>
#include <lh/memory/std.h>
#include <authum/logger.h>
#include <lh/os/fs/dir.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

enum
{
    authum_logs_buffer_size = 8192,
    authum_logs_file_capacity = 32,
    authum_logs_retention_capacity = 1024
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

typedef struct authum_logs_retention
{
    struct authum_logs_retention *next;
    authum_path_t dir;
    long keep_days;
    time_t next_check;
} authum_logs_retention_t;

static authum_logs_retention_t *retention;
static authum_usize_t retention_count;
static authum_bool_t retention_full_reported;

static authum_bool_t
authum_logs_is_open(const authum_path_t *dir, long day)
{
    for (authum_usize_t i = 0; lh_math_lt(i, authum_logs_file_capacity); i = lh_math_add_one(i))
    {
        if (files[i].file && lh_math_eq(files[i].day, day) &&
            !strcmp(authum_path_get_cstr(dir), authum_path_get_cstr(lh_addr_of(files[i].dir))))
        {
            return authum_bool_true;
        }
    }
    return authum_bool_false;
}

/* Report the first disk failure to stderr without recursing into the logger.
 * All file operations and this diagnostic stay on the writer thread. */
static void
file_error(const authum_path_t *path)
{
    if (!reported_error)
    {
        authum_i32_t saved_error = errno;
        fprintf(stderr, "authum logger: file I/O failed, path='%s': %s\n",
                authum_path_get_cstr(path), strerror(saved_error));
        reported_error = authum_bool_true;
    }
}

static authum_logs_file_t *
authum_logs_files_slot(const authum_path_t *dir)
{
    authum_logs_file_t *oldest = lh_addr_of(files[0]);
    for (authum_usize_t i = 0; lh_math_lt(i, authum_logs_file_capacity); i = lh_math_add_one(i))
    {
        authum_logs_file_t *slot = lh_addr_of(files[i]);
        if (lh_math_eq(
                strcmp(authum_path_get_cstr(lh_addr_of(slot->dir)), authum_path_get_cstr(dir)), 0))
        {
            slot->used = ++sequence;
            return slot;
        }
        if (lh_math_lt(slot->used, oldest->used))
        {
            oldest = slot;
        }
    }
    if (oldest->file)
    {
        if (lh_math_ne(fclose(oldest->file), 0))
        {
            file_error(lh_addr_of(oldest->dir));
        }
    }
    lh_memory_std_set(oldest, 0, sizeof lh_ptr_deref(oldest));
    oldest->dir = lh_ptr_deref(dir);
    oldest->used = ++sequence;
    return oldest;
}

/** Create @p dir and up to @p depth missing parents above it. */
static authum_bool_t
authum_logs_ensure_tree(const authum_path_t *dir, authum_i32_t depth)
{
    authum_path_t parent;

    if (authum_path_ensure_dir(dir))
    {
        return authum_bool_true;
    }
    return (lh_math_gt(depth, 0) && authum_path_assign_parent(lh_addr_of(parent), dir) &&
            authum_logs_ensure_tree(lh_addr_of(parent), lh_math_sub(depth, 1)) &&
            authum_path_ensure_dir(dir))
               ? authum_bool_true
               : authum_bool_false;
}

static authum_bool_t
authum_logs_channel_dir(const authum_logs_file_t *channel, authum_path_t *out)
{
    /* logs/, modules/<module>/logs/ or a journal's folder in it: create what is missing. */
    return (authum_path_assign(out, lh_addr_of(channel->dir)) && authum_logs_ensure_tree(out, 3))
               ? authum_bool_true
               : authum_bool_false;
}

/** YYYYMMDD from a `YYYY-MM-DD.log` file name, or 0. */
static long
authum_logs_file_day(const authum_char_t *name, authum_usize_t size)
{
    authum_usize_t i;

    if (lh_math_ne(size, 14u) || lh_math_ne(name[4], lh_char_map_minus) ||
        lh_math_ne(name[7], lh_char_map_minus) ||
        lh_math_ne(strncmp(lh_math_add(name, 10), ".log", 4), 0))
    {
        return 0;
    }
    for (i = 0u; lh_math_lt(i, 10u); i = lh_math_add_one(i))
    {
        if (lh_math_ne(i, 4u) && lh_math_ne(i, 7u) && !lh_char_is_digit(name[i]))
        {
            return 0;
        }
    }
    authum_i32_t year = atoi(name), month = atoi(lh_math_add(name, 5)),
                 day = atoi(lh_math_add(name, 8));
    static const authum_i32_t month_days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (lh_math_lt(year, 1970) || lh_math_lt(month, 1) || lh_math_gt(month, 12))
    {
        return 0;
    }
    authum_i32_t limit = month_days[lh_math_sub(month, 1)];
    if (lh_math_eq(month, 2) && lh_math_eq(lh_math_mod(year, 4), 0) &&
        (lh_math_ne(lh_math_mod(year, 100), 0) || lh_math_eq(lh_math_mod(year, 400), 0)))
    {
        limit = lh_math_add_one(limit);
    }
    if (lh_math_lt(day, 1) || lh_math_gt(day, limit))
    {
        return 0;
    }
    return lh_math_add(
        lh_math_add(lh_math_mul(lh_cast_static(long, year), 10000L), lh_math_mul(month, 100L)),
        day);
}

/** Delete this folder's daily files older than ::keep_days. */
static void
authum_logs_prune(const authum_path_t *dir, const struct tm *today, long keep_days)
{
    struct tm cutoff_tm = lh_ptr_deref(today);
    long cutoff;
    lh_fs_path_t lh_dir;
    lh_str_view_t text;
    lh_os_fs_dir_t listing;
    lh_os_fs_dir_entry_kind_t kind;
    lh_str_view_t entry;
    authum_char_t leaf[32];
    authum_path_t path;
    lh_ssize_t n;

    if (lh_math_le(keep_days, 0))
    {
        return;
    }
    cutoff_tm.tm_mday -= lh_cast_static(authum_i32_t, keep_days);
    cutoff_tm.tm_isdst = -1;
    if (lh_math_eq(mktime(lh_addr_of(cutoff_tm)), lh_cast_static(time_t, -1)))
    {
        return;
    }
    cutoff = lh_math_add(
        lh_math_add(
            lh_math_mul(lh_cast_static(long, (lh_math_add(cutoff_tm.tm_year, 1900))), 10000L),
            lh_math_mul(lh_cast_static(long, (lh_math_add(cutoff_tm.tm_mon, 1))), 100L)),
        cutoff_tm.tm_mday);

    lh_fs_path_init(lh_addr_of(lh_dir));
    lh_str_view_init(lh_addr_of(text), authum_path_get_cstr(dir));
    lh_fs_path_set(lh_addr_of(lh_dir), text, lh_fs_path_style_posix);
    lh_os_fs_dir_init(lh_addr_of(listing));
    if (lh_os_fs_dir_open(lh_addr_of(listing), lh_addr_of(lh_dir)))
    {
        while (lh_math_gt(
            (n = lh_os_fs_dir_read(lh_addr_of(listing), lh_addr_of(entry), lh_addr_of(kind))), 0))
        {
            long day;

            if (lh_math_eq(kind, lh_fs_kind_dir) ||
                lh_math_ge(lh_cast_static(authum_usize_t, n), sizeof leaf))
            {
                continue;
            }
            lh_memory_std_copy(leaf, lh_str_view_get_data(lh_addr_of(entry)),
                               lh_cast_static(authum_usize_t, n));
            leaf[n] = lh_char_map_nul;
            day = authum_logs_file_day(leaf, lh_cast_static(authum_usize_t, n));
            if (lh_math_ne(day, 0) && lh_math_lt(day, cutoff) && !authum_logs_is_open(dir, day) &&
                authum_path_join(lh_addr_of(path), dir, leaf))
            {
                remove(authum_path_get_cstr(lh_addr_of(path)));
            }
        }
        lh_os_fs_dir_close(lh_addr_of(listing));
    }
    lh_fs_path_deinit(lh_addr_of(lh_dir));
}

static void
authum_logs_retention_run(authum_logs_retention_t *item, time_t event_time)
{
    struct tm today;
#if defined(_WIN32)
    if (lh_math_ne(localtime_s(lh_addr_of(today), lh_addr_of(event_time)), 0))
    {
        return;
    }
#else
    if (!localtime_r(lh_addr_of(event_time), lh_addr_of(today)))
    {
        return;
    }
#endif
    authum_logs_prune(lh_addr_of(item->dir), lh_addr_of(today), item->keep_days);
    item->next_check = lh_math_add(time(authum_null), 86400);
}

static void
authum_logs_retention_register(const authum_logs_record_t *record)
{
    authum_logs_retention_t *item;
    for (item = retention; item; item = item->next)
    {
        if (!strcmp(authum_path_get_cstr(lh_addr_of(item->dir)),
                    authum_path_get_cstr(lh_addr_of(record->dir))))
        {
            break;
        }
    }
    if (!item)
    {
        if (lh_math_ge(retention_count, authum_logs_retention_capacity))
        {
            authum_logs_retention_t once = {0};
            once.dir = record->dir;
            once.keep_days = record->keep_days;
            authum_logs_retention_run(lh_addr_of(once), record->time);
            if (!retention_full_reported)
            {
                fprintf(stderr, "authum logger: retention directory limit reached; "
                                "additional directories clean only on registration/reload\n");
                retention_full_reported = authum_bool_true;
            }
            return;
        }
        item = authum_memory_alloc_zero(1, sizeof lh_ptr_deref(item));
        if (!item)
        {
            file_error(lh_addr_of(record->dir));
            return;
        }
        item->dir = record->dir;
        item->next = retention;
        retention = item;
        retention_count = lh_math_add_one(retention_count);
    }
    item->keep_days = record->keep_days;
    authum_logs_retention_run(item, record->time);
}

/** The channel's file for @p tm's day, opening (and pruning) on a new day. */
static FILE *
authum_logs_file(authum_logs_file_t *channel, const struct tm *tm)
{
    authum_path_t dir;
    authum_path_t path;
    authum_char_t leaf[16];
    long day = lh_math_add(
        lh_math_add(lh_math_mul(lh_cast_static(long, (lh_math_add(tm->tm_year, 1900))), 10000L),
                    lh_math_mul(lh_cast_static(long, (lh_math_add(tm->tm_mon, 1))), 100L)),
        tm->tm_mday);

    if (!authum_null_eq(channel->file) && lh_math_eq(channel->day, day))
    {
        return channel->file;
    }
    if (!authum_null_eq(channel->file))
    {
        if (lh_math_ne(fclose(channel->file), 0))
        {
            file_error(lh_addr_of(channel->dir));
        }
        channel->file = authum_null;
    }
    if (!strftime(leaf, sizeof leaf, "%Y-%m-%d.log", tm))
    {
        return authum_null;
    }
    if (!authum_logs_channel_dir(channel, lh_addr_of(dir)) ||
        !authum_path_join(lh_addr_of(path), lh_addr_of(dir), leaf))
    {
        file_error(lh_addr_of(channel->dir));
        return authum_null;
    }
    authum_logs_prune(lh_addr_of(dir), tm, channel->keep_days);
    channel->file = fopen(authum_path_get_cstr(lh_addr_of(path)), "a");
    if (authum_null_eq(channel->file))
    {
        file_error(lh_addr_of(path));
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
    if (record->maintenance)
    {
        authum_logs_retention_register(record);
        return;
    }
    authum_logs_file_t *channel = authum_logs_files_slot(lh_addr_of(record->dir));
    authum_logger_level_t level = record->level;
    const authum_char_t *text = record->text;
    const authum_char_t *from = record->tag[0] ? record->tag : authum_null;
    struct tm calendar;
    struct tm *tm = lh_addr_of(calendar);
    channel->keep_days = record->keep_days;
#if defined(_WIN32)
    if (lh_math_ne(localtime_s(tm, lh_addr_of(record->time)), 0))
#else
    if (!localtime_r(lh_addr_of(record->time), tm))
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
    if (lh_math_ne(record->format, authum_logger_format_raw))
    {
        fprintf(file, "[%02d:%02d:%02d] [%s] ", tm->tm_hour, tm->tm_min, tm->tm_sec,
                authum_logger_level_get_name(level));
        if (!authum_null_eq(from))
        {
            fprintf(file, "[%s] ", from);
        }
    }
    authum_usize_t length = strlen(text);
    const authum_char_t *newline =
        length && lh_math_eq(text[lh_math_sub(length, 1)], lh_char_map_lf) ? "" : "\n";
    if (lh_math_lt(fprintf(file, "%s%s", text, newline), 0))
    {
        file_error(lh_addr_of(channel->dir));
    }
    /* An error may come right before a crash: put it on disk now. */
    if (lh_math_le(level, authum_logger_level_error))
    {
        if (lh_math_ne(fflush(file), 0))
        {
            file_error(lh_addr_of(channel->dir));
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
        if (lh_math_ne(fflush(channel->file), 0))
        {
            file_error(lh_addr_of(channel->dir));
        }
    }
    channel->dirty = authum_bool_false;
}

static void
authum_logs_close_file(authum_logs_file_t *channel)
{
    if (!authum_null_eq(channel->file))
    {
        if (lh_math_ne(fclose(channel->file), 0))
        {
            file_error(lh_addr_of(channel->dir));
        }
        channel->file = authum_null;
    }
    channel->dirty = authum_bool_false;
}

void
authum_logs_files_flush(void)
{
    for (authum_usize_t i = 0; lh_math_lt(i, authum_logs_file_capacity); i = lh_math_add_one(i))
    {
        authum_logs_flush_channel(lh_addr_of(files[i]));
    }
    time_t now = time(authum_null);
    for (authum_logs_retention_t *item = retention; item; item = item->next)
    {
        if (lh_math_ge(now, item->next_check))
        {
            authum_logs_retention_run(item, now);
        }
    }
}

void
authum_logs_files_close(void)
{
    for (authum_usize_t i = 0; lh_math_lt(i, authum_logs_file_capacity); i = lh_math_add_one(i))
    {
        authum_logs_close_file(lh_addr_of(files[i]));
    }
    lh_memory_std_set(files, 0, sizeof files);
    sequence = 0;
    reported_error = authum_bool_false;
    while (retention)
    {
        authum_logs_retention_t *next = retention->next;
        authum_memory_free(retention);
        retention = next;
    }
    retention_count = 0;
    retention_full_reported = authum_bool_false;
}

void
authum_logs_files_set_root(const authum_path_t *dir)
{
    report_dir = lh_ptr_deref(dir);
}

void
authum_logs_files_dropped(authum_u32_t count)
{
    authum_logs_record_t record = {0};
    record.dir = report_dir;
    record.level = authum_logger_level_warning;
    record.time = time(authum_null);
    authum_cstr_copy(record.tag, sizeof record.tag, "logger");
    snprintf(record.text, sizeof record.text,
             "queue full: dropped %u file records; game thread did not wait",
             lh_cast_static(authum_u32_t, count));
    authum_logs_files_emit(lh_addr_of(record));
}

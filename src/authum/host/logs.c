/**
 * @file logs.c
 * @brief Log channels: one daily file per module (and per named journal)
 *        under `logs/`, levels from `authum.cfg` — see <authum/host/logs.h>.
 */

#include <authum/host/logs.h>
#include <authum/memory.h>
#include <authum/unused.h>
#include <lh/char/letter.h>
#include <lh/char/digit.h>
#include <lh/memory/std.h>
#include <lh/os/logger/daily.h>
#include <lh/os/logger/queue.h>
#include <lh/os/timestamp.h>

#include <authum/cfg.h>
#include <authum/logger.h>

#include <lh/runtime/allocator.h>

#include <lh/os/env.h>
#include <lh/runtime/logger.h>

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
    /** The id its records carry in the queue; 0 is the host. */
    lh_u32_t id;
    authum_ptr owner;
    /** The module channel this journal belongs to; null for a module or the host. */
    authum_logs_channel_t *parent;
    /** Opens of a journal by name; it closes when the last one does. */
    authum_usize_t refs;
    /** Folder name: `qproto`, or `bans` under `qproto/` for a journal. */
    authum_char_t name[authum_logs_name_max];
    /** How lines are tagged elsewhere: `qproto`, `qproto/bans`. */
    authum_char_t tag[lh_math_mul(authum_logs_name_max, 2)];
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

authum_path_t authum_logs_root;
authum_logs_mirror_fn authum_logs_mirror;
authum_logs_clock_fn authum_logs_clock;
static authum_logs_channel_t authum_logs_defaults;
static authum_i32_t authum_logs_environment_level;
static authum_bool_t authum_logs_environment_override;
authum_logs_channel_t authum_logs_host;
authum_logs_channel_t *authum_logs_head;
authum_bool_t authum_logs_ready;
static lh_os_logger_queue_t *authum_logs_queue;
static lh_os_logger_daily_t *authum_logs_daily;
/** The daily files' own callbacks, which ::authum_logs_sink forwards to. */
static lh_os_logger_sink_t authum_logs_files;
static lh_u32_t authum_logs_next_id;

enum
{
    authum_logs_queue_capacity = 1024,
    authum_logs_flush_ms = 100
};

/* --- the queue ------------------------------------------------------------ */

/** Now, in Unix seconds: the test clock when one is set. */
static lh_timestamp_t
authum_logs_now(void)
{
    lh_timestamp_t now = 0;

    if (authum_logs_clock)
    {
        return authum_logs_clock();
    }
    authum_unused(lh_os_timestamp_try_get_now(lh_addr_of(now)));
    return now;
}

/** The daily files' clock: the same one lines are stamped with. */
static lh_bool_t
authum_logs_files_clock(lh_ptr user, lh_timestamp_t *out)
{
    authum_unused(user);
    lh_ptr_deref(out) = authum_logs_now();
    return lh_bool_true;
}

/** The game server's own line: `L MM/DD/YYYY - HH:MM:SS: [level] [tag] text`. */
static lh_void
authum_logs_files_format(lh_ptr user, const lh_os_logger_record_t *record, lh_timestamp_t local,
                         lh_str_t *out)
{
    const authum_char_t *tag = lh_os_logger_record_get_tag(record);
    lh_str_t head;

    authum_unused(user);
    lh_str_init(lh_addr_of(head));
    authum_unused(
        lh_str_format(lh_addr_of(head), "L %02d/%02d/%04d - %02d:%02d:%02d: [%s] ",
                      lh_cast_static(lh_int_t, lh_timestamp_get_month(local)),
                      lh_cast_static(lh_int_t, lh_timestamp_get_day(local)),
                      lh_cast_static(lh_int_t, lh_timestamp_get_year(local)),
                      lh_cast_static(lh_int_t, lh_timestamp_get_hour(local)),
                      lh_cast_static(lh_int_t, lh_timestamp_get_minute(local)),
                      lh_cast_static(lh_int_t, lh_timestamp_get_second(local)),
                      authum_logger_level_get_name(lh_os_logger_record_get_level(record))));
    lh_str_append_str(out, lh_addr_of(head));
    if (lh_math_ne(tag[0], lh_char_map_nul))
    {
        authum_unused(lh_str_format(lh_addr_of(head), "[%s] ", tag));
        lh_str_append_str(out, lh_addr_of(head));
    }
    lh_str_deinit(lh_addr_of(head));
    lh_os_logger_daily_append_text(out, lh_os_logger_record_get_text(record));
}

static lh_void
authum_logs_sink_emit(lh_ptr user, const lh_os_logger_record_t *record)
{
    authum_unused(user);
    lh_os_logger_sink_get_emit_fn(lh_addr_of(authum_logs_files))(
        lh_os_logger_sink_get_user(lh_addr_of(authum_logs_files)), record);
}

static lh_void
authum_logs_sink_flush(lh_ptr user)
{
    authum_unused(user);
    lh_os_logger_sink_get_flush_fn(lh_addr_of(authum_logs_files))(
        lh_os_logger_sink_get_user(lh_addr_of(authum_logs_files)));
}

static lh_void
authum_logs_sink_close(lh_ptr user)
{
    authum_unused(user);
    lh_os_logger_sink_get_close_fn(lh_addr_of(authum_logs_files))(
        lh_os_logger_sink_get_user(lh_addr_of(authum_logs_files)));
}

/** A full queue is told in the host's own file, from the worker, so it is not lost too. */
static lh_void
authum_logs_sink_dropped(lh_ptr user, lh_u32_t count)
{
    lh_os_logger_record_t record;
    lh_str_t text;

    lh_str_init(lh_addr_of(text));
    authum_unused(lh_str_format(lh_addr_of(text),
                                "queue full: dropped %u file records; game thread did not wait",
                                lh_cast_static(lh_uint_t, count)));
    lh_os_logger_record_init_line(lh_addr_of(record), authum_logs_host.id,
                                  authum_logger_level_warning, authum_logs_now(), "logger",
                                  lh_str_get_data(lh_addr_of(text)));
    lh_str_deinit(lh_addr_of(text));
    authum_logs_sink_emit(user, lh_addr_of(record));
}

/** Tell the files where @p channel writes, how long it keeps and how its lines read. */
static void
authum_logs_send_channel(const authum_logs_channel_t *channel)
{
    lh_os_logger_record_t record;

    if (!authum_logs_ready)
    {
        return;
    }
    lh_os_logger_record_init_open(
        lh_addr_of(record), channel->id, authum_path_get_cstr(lh_addr_of(channel->dir)),
        lh_cast_static(lh_s32_t, channel->keep_days),
        lh_math_eq(channel->format, authum_logger_format_raw) ? lh_bool_true : lh_bool_false,
        authum_logs_now());
    authum_unused(lh_os_logger_queue_push(authum_logs_queue, lh_addr_of(record)));
}

/* --- config --------------------------------------------------------------- */

/** `off` / `none` → off; a level name → that level; anything else → @p fallback. */
authum_i32_t
authum_logs_parse_level(const authum_char_t *value, authum_i32_t fallback)
{
    authum_logger_level_t level;

    if (authum_null_eq(value))
    {
        return fallback;
    }
    if (authum_cstr_is_equal(value, "off", authum_bool_true) ||
        authum_cstr_is_equal(value, "none", authum_bool_true))
    {
        return authum_logs_level_off;
    }
    return authum_logger_level_parse(value, lh_addr_of(level)) ? lh_cast_static(authum_i32_t, level)
                                                               : fallback;
}

authum_bool_t
authum_logs_cfg_open(authum_cfg_t *cfg)
{
    authum_path_t path;

    return (authum_path_join(lh_addr_of(path), lh_addr_of(authum_logs_root),
                             "configs/authum.cfg") &&
            authum_cfg_open(cfg, lh_addr_of(path)))
               ? authum_bool_true
               : authum_bool_false;
}

/** Apply only present, valid keys. Missing/invalid values preserve inheritance. */
static void
authum_logs_apply(authum_logs_channel_t *channel, authum_cfg_t *cfg, const authum_char_t *prefix)
{
    authum_char_t key[160];
    const authum_char_t *value;
    authum_i32_t days;
#define LOG_VALUE(suffix)                                                                          \
    (lh_str_ptr_format_cstr(key, sizeof key, "%s%s", prefix, suffix),                              \
     authum_cfg_get_value(cfg, key))
    channel->level = authum_logs_parse_level(LOG_VALUE("log_level"), channel->level);
    channel->console_level =
        authum_logs_parse_level(LOG_VALUE("log_console"), channel->console_level);
    channel->host_level = authum_logs_parse_level(LOG_VALUE("log_host"), channel->host_level);
    value = LOG_VALUE("log_keep_days");
    /* Bound calendar arithmetic; reject overflow, negatives and trailing garbage. */
    if (value && authum_cstr_parse_i32(value, lh_addr_of(days)) && lh_math_ge(days, 0) &&
        lh_math_le(days, 36500))
    {
        channel->keep_days = days;
    }
    value = LOG_VALUE("log_file");
    if (value)
    {
        if (authum_cstr_is_equal(value, "on", authum_bool_false))
        {
            channel->file_enabled = authum_bool_true;
        }
        else if (authum_cstr_is_equal(value, "off", authum_bool_false))
        {
            channel->file_enabled = authum_bool_false;
        }
    }
    value = LOG_VALUE("log_format");
    if (value)
    {
        if (authum_cstr_is_equal(value, "standard", authum_bool_false))
        {
            channel->format = authum_logger_format_standard;
        }
        else if (authum_cstr_is_equal(value, "raw", authum_bool_false))
        {
            channel->format = authum_logger_format_raw;
        }
    }
#undef LOG_VALUE
}

static void
authum_logs_inherit(authum_logs_channel_t *channel, const authum_logs_channel_t *parent)
{
    channel->level = parent->level;
    channel->console_level = parent->console_level;
    channel->host_level = parent->host_level;
    channel->keep_days = parent->keep_days;
    channel->file_enabled = parent->file_enabled;
    channel->format = channel->default_format ? channel->default_format : parent->format;
}

static void
authum_logs_admin(authum_logs_channel_t *channel, authum_cfg_t *cfg, const authum_char_t *tag)
{
    authum_char_t prefix[144];
    lh_str_ptr_format_cstr(prefix, sizeof prefix, "%s.", tag);
    for (authum_char_t *p = prefix; lh_ptr_deref(p); p = lh_math_add_one(p))
    {
        if (lh_math_eq(lh_ptr_deref(p), lh_char_map_slash))
        {
            lh_ptr_deref(p) = lh_char_map_period;
        }
    }
    authum_logs_apply(channel, cfg, prefix);
}

static void
authum_logs_read_channel_level(authum_logs_channel_t *channel, authum_cfg_t *admin)
{
    authum_cfg_t local;
    const authum_logs_channel_t *module = channel->parent ? channel->parent : channel;
    /* Resolve each channel afresh: admin module overrides outrank local journal keys. */
    authum_logs_inherit(channel, lh_addr_of(authum_logs_defaults));
    if (lh_math_ne(channel, lh_addr_of(authum_logs_host)) &&
        authum_cfg_open(lh_addr_of(local), lh_addr_of(module->config)))
    {
        authum_logs_apply(channel, lh_addr_of(local), "");
        if (channel->parent)
        {
            authum_logs_admin(channel, lh_addr_of(local), channel->name);
        }
        authum_cfg_close(lh_addr_of(local));
    }
    if (admin)
    {
        if (channel->parent)
        {
            authum_logs_admin(channel, admin, channel->parent->tag);
        }
        authum_logs_admin(channel, admin, channel->tag);
    }
    if (authum_logs_environment_override)
    {
        channel->level = authum_logs_environment_level;
    }
}

/** Global keys, then every channel's own level (modules before their journals). */
void
authum_logs_read_cfg(void)
{
    authum_cfg_t cfg;
    authum_cfg_t *open = authum_logs_cfg_open(lh_addr_of(cfg)) ? lh_addr_of(cfg) : authum_null;
    authum_logs_channel_t *channel;
    authum_i32_t pass;
    lh_memory_std_set(lh_addr_of(authum_logs_defaults), 0, sizeof authum_logs_defaults);
    authum_logs_defaults.level = authum_logger_level_info;
    authum_logs_defaults.console_level = authum_logger_level_warning;
    authum_logs_defaults.host_level = authum_logger_level_warning;
    authum_logs_defaults.keep_days = 10;
    authum_logs_defaults.file_enabled = authum_bool_true;
    authum_logs_defaults.format = authum_logger_format_standard;
    if (!authum_null_eq(open))
    {
        authum_logs_apply(lh_addr_of(authum_logs_defaults), open, "");
    }
    authum_logs_read_channel_level(lh_addr_of(authum_logs_host), open);
    for (pass = 0; lh_math_lt(pass, 2); pass = lh_math_add_one(pass))
    {
        for (channel = authum_logs_head; !authum_null_eq(channel); channel = channel->next)
        {
            if (lh_math_eq((lh_math_eq(pass, 0)), authum_null_eq(channel->parent)))
            {
                authum_logs_read_channel_level(channel, open);
            }
        }
    }
    if (!authum_null_eq(open))
    {
        authum_cfg_close(open);
    }
    authum_logs_send_channel(lh_addr_of(authum_logs_host));
    for (channel = authum_logs_head; channel; channel = channel->next)
    {
        authum_logs_send_channel(channel);
    }
}

/* --- files ---------------------------------------------------------------- */

/** Copy @p name keeping it a safe directory name. */
void
authum_logs_set_name(authum_char_t *out, authum_usize_t out_size, const authum_char_t *name)
{
    authum_usize_t i;

    for (i = 0u; lh_math_ne(name[i], lh_char_map_nul) && lh_math_lt(i, lh_math_sub(out_size, 1u));
         i = lh_math_add_one(i))
    {
        authum_char_t c = name[i];
        out[i] =
            (lh_char_is_letter(c) || lh_char_is_digit(c) || lh_math_eq(c, lh_char_map_underscore) ||
             lh_math_eq(c, lh_char_map_minus) || lh_math_eq(c, lh_char_map_period))
                ? c
                : lh_char_map_underscore;
    }
    out[i] = lh_char_map_nul;
    if (lh_math_eq(i, 0u) || authum_cstr_is_equal(out, ".", authum_bool_false) ||
        authum_cstr_is_equal(out, "..", authum_bool_false))
    {
        authum_cstr_copy(out, out_size, "module");
    }
}

/** Copy caller-owned data before returning to the game thread. */
static void
authum_logs_emit(authum_logs_channel_t *channel, authum_logger_level_t level,
                 const authum_char_t *text, const authum_char_t *from)
{
    lh_os_logger_record_t record;

    lh_os_logger_record_init_line(lh_addr_of(record), channel->id, level, authum_logs_now(), from,
                                  text);
    authum_unused(lh_os_logger_queue_push(authum_logs_queue, lh_addr_of(record)));
}

/* --- channels ------------------------------------------------------------- */

authum_logs_channel_t *
authum_logs_find(authum_ptr owner)
{
    authum_logs_channel_t *channel;

    if (authum_null_eq(owner))
    {
        return lh_addr_of(authum_logs_host);
    }
    for (channel = authum_logs_head; !authum_null_eq(channel); channel = channel->next)
    {
        if (lh_math_eq(channel->owner, owner))
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
    return (lh_math_ne(channel, lh_addr_of(authum_logs_host)) &&
            lh_math_le(lh_cast_static(authum_i32_t, level), channel->host_level) &&
            authum_logs_host.file_enabled &&
            lh_math_le(lh_cast_static(authum_i32_t, level), authum_logs_host.level))
               ? authum_bool_true
               : authum_bool_false;
}

authum_bool_t
authum_logs_mirrors(const authum_logs_channel_t *channel, authum_logger_level_t level)
{
    return (lh_math_ne(authum_logs_mirror, 0) &&
            lh_math_le(lh_cast_static(authum_i32_t, level), channel->console_level))
               ? authum_bool_true
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
    return (lh_math_le(lh_cast_static(authum_i32_t, level), channel->level) &&
            lh_math_le(level, authum_logger_level_debug) &&
            (!channel->parent || lh_math_ne(channel->parent->level, authum_logs_level_off)) &&
            (channel->file_enabled || authum_logs_copies_to_host(channel, level) ||
             authum_logs_mirrors(channel, level)))
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
        lh_logger_log(lh_runtime_logger(), level, "[authum] [%s] %s",
                      authum_logger_level_get_name(level), text);
        return;
    }
    channel = authum_logs_find(owner);
    if (authum_null_eq(channel) || !authum_logs_is_enabled(owner, level))
    {
        return;
    }
    if (channel->file_enabled)
    {
        authum_logs_emit(channel, level, text, authum_null);
    }
    if (authum_logs_copies_to_host(channel, level))
    {
        authum_logs_emit(lh_addr_of(authum_logs_host), level, text, channel->tag);
    }
    if (authum_logs_mirrors(channel, level))
    {
        authum_logs_mirror(level, channel->tag, text);
    }
}

authum_logs_channel_t *
authum_logs_add(authum_ptr owner, authum_logs_channel_t *parent, const authum_char_t *name,
                const authum_char_t *module_dir)
{
    authum_path_t base = {0};
    authum_path_t modules;

    authum_logs_channel_t *channel;
    authum_cfg_t cfg;

    channel = lh_cast_reinterpret(authum_logs_channel_t *,
                                  authum_memory_alloc(sizeof lh_ptr_deref(channel)));
    if (authum_null_eq(channel))
    {
        return authum_null;
    }
    lh_memory_std_set(channel, 0, sizeof lh_ptr_deref(channel));
    channel->owner = authum_null_eq(owner) ? lh_cast_static(authum_ptr, channel) : owner;
    authum_logs_next_id = lh_math_add_one(authum_logs_next_id);
    channel->id = authum_logs_next_id;
    channel->parent = parent;
    channel->refs = 1u;
    authum_logs_set_name(channel->name, sizeof channel->name, name);
    if (authum_null_eq(parent))
    {
        authum_cstr_copy(channel->tag, sizeof channel->tag, channel->name);
    }
    else
    {
        lh_str_ptr_format_cstr(channel->tag, sizeof channel->tag, "%s/%s", parent->name,
                               channel->name);
    }
    /* journal: <module logs>/<journal>; module: <module_dir>/logs, by default
     * modules/<name>/logs */
    if (!authum_null_eq(parent))
    {
        authum_unused(
            authum_path_join(lh_addr_of(channel->dir), lh_addr_of(parent->dir), channel->name));
    }
    else if (!authum_null_eq(module_dir) && lh_math_ne(module_dir[0], 0) &&
             authum_path_set_cstr(lh_addr_of(base), module_dir))
    {
        authum_unused(authum_path_join(lh_addr_of(channel->dir), lh_addr_of(base), "logs"));
    }
    else if (authum_path_join(lh_addr_of(modules), lh_addr_of(authum_logs_root), "modules") &&
             authum_path_join(lh_addr_of(base), lh_addr_of(modules), channel->name))
    {
        authum_unused(authum_path_join(lh_addr_of(channel->dir), lh_addr_of(base), "logs"));
    }
    if (!parent)
    {
        authum_path_t configs;
        authum_char_t leaf[lh_math_add(authum_logs_name_max, 5)];
        lh_str_ptr_format_cstr(leaf, sizeof leaf, "%s.cfg", channel->name);
        if (!authum_path_join(lh_addr_of(configs), lh_addr_of(base), "configs") ||
            !authum_path_join(lh_addr_of(channel->config), lh_addr_of(configs), leaf))
        {
            authum_memory_free(channel);
            return authum_null;
        }
    }
    if (!authum_path_get_cstr(lh_addr_of(channel->dir))[0])
    {
        authum_memory_free(channel);
        return authum_null;
    }
    if (authum_logs_cfg_open(lh_addr_of(cfg)))
    {
        authum_logs_read_channel_level(channel, lh_addr_of(cfg));
        authum_cfg_close(lh_addr_of(cfg));
    }
    else
    {
        authum_logs_read_channel_level(channel, authum_null);
    }
    channel->next = authum_logs_head;
    authum_logs_head = channel;
    authum_logs_send_channel(channel);
    return channel;
}

void
authum_logs_free(authum_logs_channel_t *channel)
{
    authum_logs_channel_t **link;

    for (link = lh_addr_of(authum_logs_head); !authum_null_eq(lh_ptr_deref(link));
         link = lh_addr_of((lh_ptr_deref(link))->next))
    {
        if (lh_math_eq(lh_ptr_deref(link), channel))
        {
            lh_os_logger_record_t record;

            lh_ptr_deref(link) = channel->next;
            if (authum_logs_ready)
            {
                lh_os_logger_record_init_close(lh_addr_of(record), channel->id);
                authum_unused(lh_os_logger_queue_push(authum_logs_queue, lh_addr_of(record)));
            }
            authum_memory_free(channel);
            return;
        }
    }
}

authum_bool_t
authum_logs_open(authum_ptr owner, const authum_char_t *name, const authum_char_t *module_dir)
{
    if (authum_null_eq(owner) || authum_null_eq(name))
    {
        return authum_bool_false;
    }
    if (!authum_null_eq(authum_logs_find(owner)))
    {
        return authum_bool_true;
    }
    return !authum_null_eq(authum_logs_add(owner, authum_null, name, module_dir))
               ? authum_bool_true
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
        if (lh_math_eq(it->parent, channel))
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
        lh_math_eq(name[0], lh_char_map_nul))
    {
        return authum_null;
    }
    authum_logs_set_name(clean, sizeof clean, name);
    for (channel = authum_logs_head; !authum_null_eq(channel); channel = channel->next)
    {
        if (lh_math_eq(channel->parent, module) &&
            authum_cstr_is_equal(channel->name, clean, authum_bool_false))
        {
            channel->refs = lh_math_add_one(channel->refs);
            return channel->owner;
        }
    }
    channel = authum_logs_add(authum_null, module, clean, authum_null);
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
    if (lh_math_eq(--channel->refs, 0u))
    {
        authum_logs_free(channel);
    }
}

static authum_ptr
authum_logs_construct(authum_ptr owner, const authum_logger_options_t *options)
{
    if (!options || lh_math_lt(options->format, authum_logger_format_inherit) ||
        lh_math_gt(options->format, authum_logger_format_raw))
    {
        return authum_null;
    }
    authum_ptr journal = authum_logs_open_journal(owner, options->name);
    if (!journal)
    {
        return authum_null;
    }
    authum_logs_channel_t *channel = authum_logs_find(journal);
    if (lh_math_gt(channel->refs, 1) && lh_math_ne(channel->default_format, options->format))
    {
        authum_logs_close_journal(journal);
        return authum_null;
    }
    channel->default_format = options->format;
    authum_cfg_t cfg;
    if (authum_logs_cfg_open(lh_addr_of(cfg)))
    {
        authum_logs_read_channel_level(channel, lh_addr_of(cfg));
        authum_cfg_close(lh_addr_of(cfg));
    }
    else
    {
        authum_logs_read_channel_level(channel, authum_null);
    }
    /* The format may have changed: the files hear of it before the first line. */
    authum_logs_send_channel(channel);
    return journal;
}

/* --- lifetime ------------------------------------------------------------- */

authum_bool_t
authum_logs_init(const authum_char_t *root, authum_logs_mirror_fn mirror)
{
    authum_logs_deinit();
    if (!authum_path_set_cstr(lh_addr_of(authum_logs_root), root))
    {
        return authum_bool_false;
    }
    authum_logs_mirror = mirror;
    /* Environment is sampled once at startup, never on the write path. */
    const authum_char_t *environment = lh_os_env_get("AUTHUM_LOG_LEVEL");
    authum_logs_environment_level = authum_logs_parse_level(environment, -2);
    authum_logs_environment_override = lh_math_ne(authum_logs_environment_level, -2);
    lh_memory_std_set(lh_addr_of(authum_logs_host), 0, sizeof authum_logs_host);
    authum_logs_set_name(authum_logs_host.name, sizeof authum_logs_host.name, "authum");
    authum_cstr_copy(authum_logs_host.tag, sizeof authum_logs_host.tag, "authum");
    if (!authum_path_join(lh_addr_of(authum_logs_host.dir), lh_addr_of(authum_logs_root), "logs"))
    {
        return authum_bool_false;
    }
    authum_logs_read_cfg();
    authum_logs_daily = lh_os_logger_daily_try_create(authum_logs_files_format,
                                                      authum_logs_files_clock, authum_null);
    if (authum_null_eq(authum_logs_daily))
    {
        return authum_bool_false;
    }
    lh_os_logger_daily_copy_sink(authum_logs_daily, lh_addr_of(authum_logs_files));
    {
        lh_os_logger_sink_t sink;

        lh_os_logger_sink_set_emit_fn(lh_addr_of(sink), authum_logs_sink_emit);
        lh_os_logger_sink_set_flush_fn(lh_addr_of(sink), authum_logs_sink_flush);
        lh_os_logger_sink_set_close_fn(lh_addr_of(sink), authum_logs_sink_close);
        lh_os_logger_sink_set_dropped_fn(lh_addr_of(sink), authum_logs_sink_dropped);
        lh_os_logger_sink_set_user(lh_addr_of(sink), authum_null);
        authum_logs_queue = lh_os_logger_queue_try_create(authum_logs_queue_capacity,
                                                          authum_logs_flush_ms, lh_addr_of(sink));
    }
    if (authum_null_eq(authum_logs_queue))
    {
        lh_os_logger_daily_destroy(authum_logs_daily);
        authum_logs_daily = authum_null;
        return authum_bool_false;
    }
    authum_logs_ready = authum_bool_true;
    authum_logs_send_channel(lh_addr_of(authum_logs_host));
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

authum_bool_t
authum_logs_sync(authum_u32_t timeout_ms)
{
    return authum_logs_ready ? lh_os_logger_queue_sync(authum_logs_queue, timeout_ms)
                             : authum_bool_false;
}

void
authum_logs_deinit(void)
{
    while (!authum_null_eq(authum_logs_head))
    {
        authum_logs_free(authum_logs_head);
    }
    lh_os_logger_queue_destroy(authum_logs_queue);
    authum_logs_queue = authum_null;
    lh_os_logger_daily_destroy(authum_logs_daily);
    authum_logs_daily = authum_null;
    authum_logs_mirror = 0;
    authum_logs_ready = authum_bool_false;
}

void
authum_logs_set_clock(authum_logs_clock_fn clock)
{
    authum_logs_clock = clock;
}

const authum_logger_ops_t authum_logs_ops_table = {
    .write = authum_logs_write,
    .is_enabled = authum_logs_is_enabled,
    .open = authum_logs_open_journal,
    .close = authum_logs_close_journal,
    .construct = authum_logs_construct,
};

const authum_logger_ops_t *
authum_logs_ops(void)
{
    return lh_addr_of(authum_logs_ops_table);
}

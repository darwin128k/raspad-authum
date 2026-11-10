/**
 * @file logs.h
 * @brief Host side of the log channels — implemented in `authum` only.
 *
 * Channel registry and console callbacks belong to the host/game thread.
 * File writes, rotation, flushing and retention run on the worker of an LH log
 * queue (`lh/os/logger/queue.h`) into LH daily files (`lh/os/logger/daily.h`).
 * The queue holds 1024 records, at most 1023 text bytes each. On full it drops
 * the newest and the worker reports the loss. Disk stalls never block a write.
 * Formatting and console mirroring remain on the caller thread. Workers use
 * logger buffers and deliver them through the task completion as before.
 * Errors flush when the worker handles them; process crashes can lose queued lines.
 *
 * One registry of channels: the host's own (`authum`, owner null), writing
 * `<root>/logs/YYYY-MM-DD.log`, and one per loaded module, writing
 * `modules/<module>/logs/YYYY-MM-DD.log` (a new file when the date changes). Levels come from
 * `<root>/configs/authum.cfg`:
 *
 * ```
 * log_level = info          # what reaches the files
 * log_console = warning     # what also goes to the server console
 * log_keep_days = 30        # delete daily files older than this (0 = keep)
 * qproto.log_level = debug  # one module (or `authum.log_level` for the host)
 * qproto.bans.log_level = info  # one journal of a module
 * ```
 *
 * A module may also open named journals: `modules/<module>/logs/<name>/YYYY-MM-DD.log`.
 * The file is re-read on every map change (::authum_logs_reload).
 *
 * Module configs/<module>.cfg inherits those defaults. Explicit prefixed keys
 * in authum.cfg override local settings. All log_* keys can be set per module
 * and per journal; see docs/logging.md. log_level=off disables every destination.
 * Warnings and worse copy to the host by default; log_host changes that threshold.
 */

#ifndef AUTHUM_HOST_LOGS_H
#define AUTHUM_HOST_LOGS_H

#include <authum/logger/ops.h>
#include <lh/core/char/letter.h>
#include <lh/core/char/digit.h>
#include <lh/core/memory/std.h>
#include <lh/os/logger/daily.h>
#include <lh/os/logger/queue.h>
#include <lh/os/timestamp.h>
#include <lh/core/runtime/allocator.h>
#include <lh/os/env.h>
#include <lh/core/runtime/logger.h>
#include <authum/cfg.h>

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
    /** The source mark of the records: `QPROTO`, `QPROTO/bans` for a journal. */
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

AUTHUM_EXTERN_C_BEGIN

/** Where console-level lines go besides the files (the server console). */
typedef void (*authum_logs_mirror_fn)(authum_logger_level_t level, const authum_char_t *name,
                                      const authum_char_t *text);

/** Wall clock, in Unix seconds (UTC); replaceable so tests can cross midnight. */
typedef authum_i64_t (*authum_logs_clock_fn)(void);

/**
 * Read `<root>/configs/authum.cfg` and open the host channel. @p root is the
 * directory of `authum` (`addons/authum`). @p mirror may be null.
 */
authum_bool_t
authum_logs_init(const authum_char_t *root, authum_logs_mirror_fn mirror);

/** Close every channel. Lines written afterwards go to stderr. */
void
authum_logs_deinit(void);

/** Ops for ::authum_logger_channel_t; owner is the module, null = host. */
const authum_logger_ops_t *
authum_logs_ops(void);

/**
 * Start a channel for module @p owner named @p name (its `log_level` is read
 * now), writing under `@p module_dir/logs` — or `modules/<name>/logs` when
 * @p module_dir is null.
 *
 * @p tag is the source mark the records carry (`QPROTO`); an empty @p tag
 * falls back to @p name.
 */
authum_bool_t
authum_logs_open(authum_ptr owner, const authum_char_t *name, const authum_char_t *tag,
                 const authum_char_t *module_dir);

/** End @p owner's channel (after the module's `stop`). */
void
authum_logs_close(authum_ptr owner);

void
authum_logs_write(authum_ptr owner, authum_logger_level_t level, const authum_char_t *text);

authum_bool_t
authum_logs_is_enabled(authum_ptr owner, authum_logger_level_t level);

/** Re-read `authum.cfg`: global keys and every open channel's level. */
void
authum_logs_reload(void);

/** Blocking file visibility barrier for tests/admin, never a game frame. */
authum_bool_t
authum_logs_sync(authum_u32_t timeout_ms);

/** Journal @p name of module @p owner; see ::authum_logger_ops_t.open. */
authum_ptr
authum_logs_open_journal(authum_ptr owner, const authum_char_t *name);

void
authum_logs_close_journal(authum_ptr journal);

void
authum_logs_set_clock(authum_logs_clock_fn clock);

/**
 * @brief Now, in Unix seconds: the test clock when one is set.
 */
lh_timestamp_t
authum_logs_now(void);

/**
 * @brief The daily files' clock: the same one lines are stamped with.
 */
lh_bool_t
authum_logs_files_clock(lh_ptr user, lh_timestamp_t *out);

/**
 * @brief The game server's own line: `L MM/DD/YYYY - HH:MM:SS: [level] [tag] text`.
 */
lh_void
authum_logs_files_format(lh_ptr user, const lh_os_logger_record_t *record, lh_timestamp_t local,
                         lh_str_t *out);

/**
 * @brief Hand a record to the daily files: the log queue's emit.
 */
lh_void
authum_logs_sink_emit(lh_ptr user, const lh_os_logger_record_t *record);

/**
 * @brief Flush the daily files: the log queue's flush.
 */
lh_void
authum_logs_sink_flush(lh_ptr user);

/**
 * @brief Close the daily files: the log queue's close.
 */
lh_void
authum_logs_sink_close(lh_ptr user);

/**
 * @brief A full queue is told in the host's own file, from the worker, so it is not lost too.
 */
lh_void
authum_logs_sink_dropped(lh_ptr user, lh_u32_t count);

/**
 * @brief Tell the files where @p channel writes, how long it keeps and how its lines read.
 */
void
authum_logs_send_channel(const authum_logs_channel_t *channel);

/**
 * @brief Apply only present, valid keys. Missing/invalid values preserve inheritance.
 */
void
authum_logs_apply(authum_logs_channel_t *channel, authum_cfg_t *cfg, const authum_char_t *prefix);

/**
 * @brief Copy the levels, retention and format @p channel takes from @p parent.
 */
void
authum_logs_inherit(authum_logs_channel_t *channel, const authum_logs_channel_t *parent);

/**
 * @brief Apply the `<name>.log_*` keys of the admin config @p cfg to @p channel.
 */
void
authum_logs_admin(authum_logs_channel_t *channel, authum_cfg_t *cfg, const authum_char_t *name);

/**
 * @brief Resolve @p channel's settings: defaults, its module config, then the admin's keys.
 */
void
authum_logs_read_channel_level(authum_logs_channel_t *channel, authum_cfg_t *admin);

/**
 * @brief Copy caller-owned data before returning to the game thread.
 */
void
authum_logs_emit(authum_logs_channel_t *channel, authum_logger_level_t level,
                 const authum_char_t *text, const authum_char_t *from);

/**
 * @brief Open a journal of @p owner as @p options say: the logger table's construct.
 */
authum_ptr
authum_logs_construct(authum_ptr owner, const authum_logger_options_t *options);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_HOST_LOGS_H */

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
 */
authum_bool_t
authum_logs_open(authum_ptr owner, const authum_char_t *name, const authum_char_t *module_dir);

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

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_HOST_LOGS_H */

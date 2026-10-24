/**
 * @file logs.h
 * @brief Host side of the log channels — implemented in `authum` only.
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
 * A module's warnings and worse are copied into the `authum` channel, tagged
 * `[module]`, so one file shows every problem on the server.
 */

#ifndef AUTHUM_HOST_LOGS_H
#define AUTHUM_HOST_LOGS_H

#include <authum/bool.h>
#include <authum/char.h>
#include <authum/extern.h>
#include <authum/logger/level.h>
#include <authum/logger/ops.h>
#include <authum/ptr.h>

#include <time.h>

AUTHUM_EXTERN_C_BEGIN

/** Where console-level lines go besides the files (the server console). */
typedef void (*authum_logs_mirror_fn)(authum_logger_level_t level, const authum_char_t *name,
                                      const authum_char_t *text);

/** Wall clock; replaceable so tests can cross midnight. */
typedef time_t (*authum_logs_clock_fn)(void);

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

/**
 * Put buffered lines on disk. Lines are buffered (errors and worse are
 * flushed at once); the host calls this every server frame.
 */
void
authum_logs_flush(void);

/** Journal @p name of module @p owner; see ::authum_logger_ops_t.open. */
authum_ptr
authum_logs_open_journal(authum_ptr owner, const authum_char_t *name);

void
authum_logs_close_journal(authum_ptr journal);

void
authum_logs_set_clock(authum_logs_clock_fn clock);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_HOST_LOGS_H */

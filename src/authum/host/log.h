/**
 * @file log.h
 * @brief Authum's own log on ::lh_runtime_logger, routed by <authum/host/logs.h>.
 *
 * Host lines (load OK/FAIL, game library, gate) land in
 * `logs/YYYY-MM-DD.log` beside the host image, with modules' warnings and
 * errors. Console-level lines can also be mirrored to the server console by
 * the host (::authum_log_init_sink).
 */

#ifndef AUTHUM_HOST_LOG_H
#define AUTHUM_HOST_LOG_H

#include <authum/host/logs.h>
#include <lh/fs/path.h>

AUTHUM_EXTERN_C_BEGIN

/**
 * @brief Read `authum.cfg` under @p image_dir, open the log channels and wire
 *        ::lh_runtime_logger.
 *
 * @p image_dir is the directory that holds `authum.so` (not `modules/`).
 * Safe to call again: the previous channels are closed first.
 */
lh_bool_t
authum_log_init(const lh_fs_path_t *image_dir);

/**
 * @brief The same, plus a mirror: console-level lines (warning and worse)
 *        also go to @p sink — the host's way to reach the server console.
 *
 * @p sink may be null (no mirror).
 */
lh_bool_t
authum_log_init_sink(const lh_fs_path_t *image_dir, authum_logs_mirror_fn sink);

/**
 * @brief Close every channel and leave ::lh_runtime_logger on stderr (stdlib).
 */
void
authum_log_deinit(void);

void
authum_log_info(const authum_char_t *fmt, ...);

void
authum_log_warning(const authum_char_t *fmt, ...);

void
authum_log_error(const authum_char_t *fmt, ...);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_HOST_LOG_H */

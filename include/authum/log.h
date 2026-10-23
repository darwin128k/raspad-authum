/**
 * @file log.h
 * @brief Authum's own log on ::lh_runtime_logger, routed by <authum/logger/host.h>.
 *
 * Host lines (load OK/FAIL, attach, gate) land in
 * `logs/YYYY-MM-DD.log` beside the plugin image, with modules'
 * warnings and errors; console-level lines also go through Metamod.
 */

#ifndef AUTHUM_LOG_H
#define AUTHUM_LOG_H

#include <authum/char.h>
#include <lh/bool.h>
#include <lh/compiler/extern/c.h>
#include <lh/fs/path.h>

LH_COMPILER_EXTERN_C_BEGIN

/**
 * @brief Read `authum.cfg` under @p image_dir, open the log channels and wire
 *        ::lh_runtime_logger.
 *
 * @p image_dir is the directory that holds `authum_mm.so` (not `modules/`).
 * Safe to call again: the previous channels are closed first.
 */
lh_bool_t
authum_log_init(const lh_fs_path_t *image_dir);

/**
 * @brief Close every channel and leave ::lh_runtime_logger on stderr (stdlib).
 */
void
authum_log_deinit(void);

void
authum_log_info(const char *fmt, ...);
void
authum_log_warning(const char *fmt, ...);
void
authum_log_error(const char *fmt, ...);

LH_COMPILER_EXTERN_C_END

#endif /* AUTHUM_LOG_H */

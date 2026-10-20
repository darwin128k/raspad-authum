/**
 * @file log.h
 * @brief Authum host log on ::lh_runtime_logger — file + Metamod mirror.
 *
 * Opens `authum.log` beside the plugin image. Load OK/FAIL and attach noise
 * go there so you do not need `meta list` or a client connect to see what
 * happened.
 */

#ifndef AUTHUM_LOG_H
#define AUTHUM_LOG_H

#include <lh/bool.h>
#include <lh/compiler/extern/c.h>
#include <lh/fs/path.h>

LH_COMPILER_EXTERN_C_BEGIN

/**
 * @brief Open `authum.log` under @p image_dir and wire ::lh_runtime_logger.
 *
 * @p image_dir is the directory that holds `authum_mm.so` (not `modules/`).
 * Safe to call once; a second call closes the previous file.
 */
lh_bool_t
authum_log_init(const lh_fs_path_t *image_dir);

/**
 * @brief Close the log file and leave ::lh_runtime_logger on stderr (stdlib).
 */
void
authum_log_deinit(void);

void authum_log_info(const char *fmt, ...);
void authum_log_error(const char *fmt, ...);

/**
 * @brief One preformatted line for child modules (dlsym from the host image).
 *
 * @p is_error non-zero → error channel. Used by qproto so its log lands in
 * `authum.log` (child .so has its own ::lh_runtime_logger instance).
 */
#ifndef AUTHUM_MODULE_EXPORT
#    if defined(_WIN32) || defined(__CYGWIN__)
#        define AUTHUM_MODULE_EXPORT __declspec(dllexport)
#    elif defined(__GNUC__)
#        define AUTHUM_MODULE_EXPORT __attribute__((visibility("default")))
#    else
#        define AUTHUM_MODULE_EXPORT
#    endif
#endif

AUTHUM_MODULE_EXPORT void
authum_host_log(int is_error, const char *line);

LH_COMPILER_EXTERN_C_END

#endif /* AUTHUM_LOG_H */

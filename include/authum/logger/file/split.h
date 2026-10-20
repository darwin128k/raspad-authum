/**
 * @file split.h
 * @brief How a module-private log file is named under the module directory.
 */

#ifndef AUTHUM_LOGGER_FILE_SPLIT_H
#define AUTHUM_LOGGER_FILE_SPLIT_H

#include <authum/char.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @enum authum_logger_file_split
     * @brief Private-file layout for ::authum_logger_open_module_file.
     */
    enum authum_logger_file_split
    {
        /** One `<module>/<tag>.log` — append across restarts. */
        authum_logger_file_split_full = 0,
        /** New file each open: `<module>/logs/<tag>-YYYYMMDD-HHMMSS.log`. */
        authum_logger_file_split_session = 1
    };
    typedef enum authum_logger_file_split authum_logger_file_split_t;

    /**
     * @brief Map cfg text (`full` / `session` / `sessions` / `1`) to a split.
     *        Unknown or empty → ::authum_logger_file_split_full.
     */
    authum_logger_file_split_t
    authum_logger_file_split_parse(const authum_char_t *value);

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_LOGGER_FILE_SPLIT_H */

/**
 * @file private.h
 * @brief Private to `file.c`: the functions below its public API.
 *
 * Not installed: modules never see these.
 */

#ifndef AUTHUM_API_FILE_PRIVATE_H
#define AUTHUM_API_FILE_PRIVATE_H

#include <authum/file.h>
#include <lh/core/cast/const.h>
#include <lh/core/char/map.h>
#include <lh/os/fs/file.h>
#include <lh/os/system/fs/path.h>

AUTHUM_EXTERN_C_BEGIN

/**
 * @brief The lh file behind @p self, or null when the file is closed.
 */
lh_os_fs_file_t *
authum_file_backend(const authum_file_t *self);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_API_FILE_PRIVATE_H */

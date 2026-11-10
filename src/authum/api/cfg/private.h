/**
 * @file private.h
 * @brief Private to `cfg.c`: the functions below its public API.
 *
 * Not installed: modules never see these.
 */

#ifndef AUTHUM_API_CFG_PRIVATE_H
#define AUTHUM_API_CFG_PRIVATE_H

#include <authum/cfg.h>
#include <lh/core/cast/const.h>
#include <lh/core/char/map.h>
#include <lh/core/memory/std.h>
#include <lh/os/fs/file.h>
#include <lh/os/system/fs/path.h>
#include <lh/core/str/split/line.h>
#include <lh/core/ini/parser.h>

AUTHUM_EXTERN_C_BEGIN

/**
 * @brief * Read the whole file into a block of our own.  The file is read once, when it is opened:
 * every getter scans the text from its first line again, the way the old stream was rewound and
 * read. A file that cannot be opened or read leaves the config closed.
 */
authum_bool_t
authum_cfg_read_file(const lh_fs_path_t *path, authum_char_t **out_text, authum_usize_t *out_size);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_API_CFG_PRIVATE_H */

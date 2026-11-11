/**
 * @file sep.h
 * @brief Path separator for Authum module path helpers.
 *
 * Modules always join with `/`. Host-filled ::authum_env_t strings use the
 * platform-native form (via lh inside `authum`); `/` works on every
 * platform with ::authum_file_open.
 */

#ifndef AUTHUM_PATH_SEP_H
#define AUTHUM_PATH_SEP_H

#include <authum/char.h>

/** Canonical join separator for Authum path helpers. */
#define authum_path_sep ((authum_char_t)'/')

#endif /* AUTHUM_PATH_SEP_H */

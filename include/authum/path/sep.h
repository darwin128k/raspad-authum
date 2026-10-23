/**
 * @file sep.h
 * @brief Path separator for Authum module path helpers.
 *
 * Modules always join with `/`. Host-filled ::authum_env_t strings use the
 * platform-native form (via lh inside `authum`); `/` still works with
 * `fopen` on Windows HLDS.
 */

#ifndef AUTHUM_PATH_SEP_H
#define AUTHUM_PATH_SEP_H

#include <authum/char.h>

/** Canonical join separator for Authum path helpers. */
#define authum_path_sep ((authum_char_t)'/')

#endif /* AUTHUM_PATH_SEP_H */

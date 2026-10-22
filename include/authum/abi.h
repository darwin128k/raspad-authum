/**
 * @file abi.h
 * @brief How Authum's function tables grow without breaking modules.
 *
 * Every table the host hands a module (::authum_host_t, ::authum_engine_t,
 * ::authum_crypto_t, the events / sandbox / commands ops) starts with
 * `authum_usize_t size` = `sizeof` the table as the host built it. New
 * functions are only ever appended. The client wrappers call a function only
 * when ::AUTHUM_ABI_HAS says the host's table reaches it; otherwise they
 * return their "not available" value. So:
 *
 * - a module built against newer headers runs on an older host (the new
 *   functions just report "not available");
 * - an older module runs on a newer host (it never looks past what it knows).
 *
 * Adding a function is a minor ::AUTHUM_MODULE_ABI bump. Removing or changing
 * one, or reordering fields, is a major bump.
 */

#ifndef AUTHUM_ABI_H
#define AUTHUM_ABI_H

#include <authum/size.h>

/**
 * @def AUTHUM_ABI_FITS
 * @brief True when the non-null table @p table, sized by its `size` field,
 *        reaches field @p field (set or not).
 */
#define AUTHUM_ABI_FITS(table, field)                                                              \
    ((authum_usize_t)((const char *)&(table)->field - (const char *)(table)) +                     \
         sizeof((table)->field) <=                                                                 \
     (table)->size)

/**
 * @def AUTHUM_ABI_HAS
 * @brief ::AUTHUM_ABI_FITS and the field is set (non-null function pointer).
 */
#define AUTHUM_ABI_HAS(table, field) (AUTHUM_ABI_FITS(table, field) && (table)->field != 0)

#endif /* AUTHUM_ABI_H */

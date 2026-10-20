/**
 * @file level.h
 * @brief Log severity for ::authum_logger_t (PSR-3 names, like lh).
 */

#ifndef AUTHUM_LOGGER_LEVEL_H
#define AUTHUM_LOGGER_LEVEL_H

#include <authum/byte.h>

/** System unusable. */
#define authum_logger_level_emergency 0
/** Action must be taken immediately. */
#define authum_logger_level_alert 1
/** Critical conditions. */
#define authum_logger_level_critical 2
/** Runtime errors. */
#define authum_logger_level_error 3
/** Exceptional occurrences that are not errors. */
#define authum_logger_level_warning 4
/** Normal but significant events. */
#define authum_logger_level_notice 5
/** Interesting events. */
#define authum_logger_level_info 6
/** Detailed debug information. */
#define authum_logger_level_debug 7

/**
 * @typedef authum_logger_level_t
 * @brief One of the `authum_logger_level_*` constants (0–7).
 */
typedef authum_byte_t authum_logger_level_t;

#endif /* AUTHUM_LOGGER_LEVEL_H */

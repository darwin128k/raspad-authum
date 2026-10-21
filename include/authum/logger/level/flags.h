/**
 * @file flags.h
 * @brief Which ::authum_logger_level_t values a logger emits.
 */

#ifndef AUTHUM_LOGGER_LEVEL_FLAGS_H
#define AUTHUM_LOGGER_LEVEL_FLAGS_H

#include <authum/byte.h>
#include <authum/logger/level.h>

#define authum_logger_level_flags_none 0u
#define authum_logger_level_flags_emergency (1u << authum_logger_level_emergency)
#define authum_logger_level_flags_alert (1u << authum_logger_level_alert)
#define authum_logger_level_flags_critical (1u << authum_logger_level_critical)
#define authum_logger_level_flags_error (1u << authum_logger_level_error)
#define authum_logger_level_flags_warning (1u << authum_logger_level_warning)
#define authum_logger_level_flags_notice (1u << authum_logger_level_notice)
#define authum_logger_level_flags_info (1u << authum_logger_level_info)
#define authum_logger_level_flags_debug (1u << authum_logger_level_debug)

#define authum_logger_level_flags_all                                                              \
    (authum_logger_level_flags_emergency | authum_logger_level_flags_alert |                       \
     authum_logger_level_flags_critical | authum_logger_level_flags_error |                        \
     authum_logger_level_flags_warning | authum_logger_level_flags_notice |                        \
     authum_logger_level_flags_info | authum_logger_level_flags_debug)

/**
 * @typedef authum_logger_level_flags_t
 * @brief Bitmask of enabled levels.
 */
typedef authum_byte_t authum_logger_level_flags_t;

#endif /* AUTHUM_LOGGER_LEVEL_FLAGS_H */

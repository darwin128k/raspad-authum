/**
 * @file time.h
 * @brief The wall clock for Authum modules (no lh).
 *
 * The monotonic clock (a timer, a deadline, a rate limit) is the host's
 * clock service — see <authum/clock.h>. This is the calendar time: seconds
 * since the epoch, UTC.
 */

#ifndef AUTHUM_TIME_H
#define AUTHUM_TIME_H

#include <authum/char.h>
#include <authum/extern.h>
#include <authum/size.h>

AUTHUM_EXTERN_C_BEGIN

/**
 * @brief Seconds since the epoch (UTC).
 *
 * @return 0 when the system clock cannot be read.
 */
authum_i64_t
authum_time_now_seconds(void);

/**
 * @brief Write @p seconds as `YYYY-MM-DD HH:MM:SS UTC` into @p dst.
 *
 * @param seconds Seconds since the epoch (UTC).
 * @param dst     Destination buffer.
 * @param dst_size Capacity of @p dst in characters, the NUL included.
 *
 * @return Characters written, the NUL not counted; 0 when @p dst is null, has
 *         no room, or the moment cannot be broken into parts.
 */
authum_usize_t
authum_time_format_utc(authum_i64_t seconds, authum_char_t *dst, authum_usize_t dst_size);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_TIME_H */

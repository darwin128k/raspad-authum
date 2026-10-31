#include <authum/time.h>

#include <authum/null.h>

#include <lh/os/timestamp.h>

authum_i64_t
authum_time_now_seconds(void)
{
    lh_timestamp_t now;

    if (!lh_os_timestamp_try_get_now(lh_addr_of(now)))
    {
        return 0;
    }
    return lh_cast_static(authum_i64_t, now);
}

authum_usize_t
authum_time_format_utc(authum_i64_t seconds, authum_char_t *dst, authum_usize_t dst_size)
{
    lh_timestamp_t moment;
    lh_date_t date;
    lh_time_t clock;

    if (authum_null_eq(dst) || lh_math_le(dst_size, 24u))
    {
        return 0u;
    }
    moment = lh_cast_static(lh_timestamp_t, seconds);
    lh_timestamp_get_date(moment, lh_addr_of(date));
    lh_timestamp_get_time(moment, lh_addr_of(clock));
    lh_date_format_by(lh_addr_of(date), dst, 11u, '-');
    dst[10] = ' ';
    lh_time_format(lh_addr_of(clock), dst + 11, dst_size - 11u);
    dst[19] = ' ';
    dst[20] = 'U';
    dst[21] = 'T';
    dst[22] = 'C';
    dst[23] = '\0';
    return 23u;
}

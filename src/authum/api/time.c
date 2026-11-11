#include <authum/time.h>

#include <authum/null.h>

#include <lh/core/char/map.h>
#include <lh/core/str/format/text.h>
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
    if (authum_null_eq(dst))
    {
        return 0u;
    }
    lh_datetime_t moment;
    lh_timestamp_to_datetime(lh_cast_static(lh_timestamp_t, seconds), lh_addr_of(moment));
    const lh_usize_t size =
        lh_datetime_format_by(lh_addr_of(moment), dst, dst_size, lh_char_map_minus);
    if (lh_math_is_zero(size) || lh_math_ge(size, dst_size))
    {
        return 0u;
    }
    /* The zone after the moment, and the NUL after it. */
    const lh_usize_t zone = lh_str_ptr_format_cstr(lh_ptr_add_by_offset(authum_char_t, dst, size),
                                                   lh_math_sub(dst_size, size), " UTC");
    return lh_math_is_zero(zone) ? 0u : lh_math_add(size, zone);
}

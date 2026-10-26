#include <authum/clock.h>

authum_i64_t
authum_clock_monotonic_ms(const authum_clock_t *clock)
{
    return clock && AUTHUM_ABI_HAS(clock, monotonic_ms) ? clock->monotonic_ms() : 0;
}

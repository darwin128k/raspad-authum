#include <authum/clock.h>
#include <authum/null.h>

authum_i64_t
authum_clock_monotonic_ms(const authum_clock_t *clock)
{
    return clock && !authum_null_eq((clock)->monotonic_ms) ? clock->monotonic_ms() : 0;
}

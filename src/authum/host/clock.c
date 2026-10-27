#include <authum/host/clock.h>
#include <lh/os/tick.h>

/**
 * @brief Read monotonic milliseconds through LH without platform code in Authum.
 */
static authum_i64_t
monotonic_ms(void)
{
    return (authum_i64_t)lh_os_tick_ms64();
}

/**
 * @brief Return the immutable host clock facade.
 */
const authum_clock_t *
authum_clock_host(void)
{
    static const authum_clock_t clock = {sizeof clock, monotonic_ms};
    return &clock;
}

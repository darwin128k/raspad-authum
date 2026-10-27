#include <authum/host/clock.h>
#include <lh/os/tick.h>

static authum_i64_t
monotonic_ms(void)
{
    return (authum_i64_t)lh_os_tick_ms64();
}

const authum_clock_t *
authum_clock_host(void)
{
    static const authum_clock_t clock = {sizeof clock, monotonic_ms};
    return &clock;
}

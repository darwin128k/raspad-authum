#include <authum/host/clock.h>
#if defined(_WIN32)
#    include <windows.h>
#else
#    include <time.h>
#endif
static authum_i64_t
monotonic_ms(void)
{
#if defined(_WIN32)
    LARGE_INTEGER counter, frequency;
    if (!QueryPerformanceCounter(&counter) || !QueryPerformanceFrequency(&frequency))
    {
        return 0;
    }
    return counter.QuadPart / frequency.QuadPart * 1000 +
           counter.QuadPart % frequency.QuadPart * 1000 / frequency.QuadPart;
#else
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now))
    {
        return 0;
    }
    return (authum_i64_t)now.tv_sec * 1000 + now.tv_nsec / 1000000;
#endif
}

const authum_clock_t *
authum_clock_host(void)
{
    static const authum_clock_t clock = {sizeof clock, monotonic_ms};
    return &clock;
}

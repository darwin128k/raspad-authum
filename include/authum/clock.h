#ifndef AUTHUM_CLOCK_H
#define AUTHUM_CLOCK_H
#include <authum/abi.h>
#include <authum/char.h>
#include <authum/extern.h>
AUTHUM_EXTERN_C_BEGIN

typedef struct authum_clock
{
    authum_usize_t size;
    authum_i64_t (*monotonic_ms)(void);
} authum_clock_t;

/** Elapsed monotonic milliseconds, not calendar time. Safe on workers.
 * Zero means unavailable. Never use game/map time for network deadlines. */
authum_i64_t
authum_clock_monotonic_ms(const authum_clock_t *clock);
AUTHUM_EXTERN_C_END
#endif

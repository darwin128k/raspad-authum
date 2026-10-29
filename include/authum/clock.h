#ifndef AUTHUM_CLOCK_H
#define AUTHUM_CLOCK_H

#include <authum/size.h>
#include <authum/char.h>
#include <authum/extern.h>

typedef struct authum_clock
{
    authum_i64_t (*monotonic_ms)(void);
} authum_clock_t;

AUTHUM_EXTERN_C_BEGIN

/** Elapsed monotonic milliseconds, not calendar time. Safe on workers.
 * Zero means unavailable. Never use game/map time for network deadlines. */
authum_i64_t
authum_clock_get_monotonic_ms(const authum_clock_t *clock);

AUTHUM_EXTERN_C_END

#endif

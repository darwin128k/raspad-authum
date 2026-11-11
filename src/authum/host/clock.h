#ifndef AUTHUM_HOST_CLOCK_H
#define AUTHUM_HOST_CLOCK_H

#include <authum/clock.h>
#include <lh/core/util/ptr.h>

AUTHUM_EXTERN_C_BEGIN

/**
 * @brief Monotonic milliseconds of the host, read through lh's tick.
 *
 * The `monotonic_ms` of ::authum_clock_host; Authum names no platform clock.
 */
authum_i64_t
authum_clock_host_get_monotonic_ms(void);

/**
 * @brief Return the immutable host monotonic-clock facade.
 */
const authum_clock_t lh_ptr_deref(authum_clock_host(void));

AUTHUM_EXTERN_C_END

#endif

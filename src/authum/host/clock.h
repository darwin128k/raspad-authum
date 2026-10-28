#ifndef AUTHUM_HOST_CLOCK_H
#define AUTHUM_HOST_CLOCK_H

#include <authum/clock.h>
#include <lh/util/ptr.h>

AUTHUM_EXTERN_C_BEGIN

/**
 * @brief Return the immutable host monotonic-clock facade.
 */
const authum_clock_t lh_ptr_deref(authum_clock_host(void));

AUTHUM_EXTERN_C_END

#endif

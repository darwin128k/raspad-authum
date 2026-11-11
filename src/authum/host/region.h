/**
 * @file region.h
 * @brief Host side of ::authum_region_t — implemented in `authum` only.
 *
 * The loader creates one root per module before `start` and destroys it after
 * `stop`. Modules must not call these; they use <authum/region.h>.
 */

#ifndef AUTHUM_HOST_REGION_H
#define AUTHUM_HOST_REGION_H

#include <lh/core/util/ptr.h>
#include <authum/region/ops.h>

AUTHUM_EXTERN_C_BEGIN

const authum_region_ops_t lh_ptr_deref(authum_region_host_ops(void));

/** New empty root (owns nothing yet). */
authum_ptr
authum_region_host_root_create(void);

/** Free @p root and everything any module allocated under it. */
void
authum_region_host_root_destroy(authum_ptr root);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_HOST_REGION_H */

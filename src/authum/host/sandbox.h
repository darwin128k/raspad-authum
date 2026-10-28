/**
 * @file sandbox.h
 * @brief Host side of ::authum_sandbox_t — implemented in `authum` only.
 *
 * The loader creates one root per module before `start` and destroys it after
 * `stop`. Modules must not call these; they use <authum/sandbox.h>.
 */

#ifndef AUTHUM_HOST_SANDBOX_H
#define AUTHUM_HOST_SANDBOX_H

#include <authum/extern.h>
#include <lh/util/ptr.h>
#include <authum/ptr.h>
#include <authum/sandbox/ops.h>

AUTHUM_EXTERN_C_BEGIN

const authum_sandbox_ops_t lh_ptr_deref(authum_sandbox_host_ops(void));

/** New empty root (owns nothing yet). */
authum_ptr
authum_sandbox_host_root_create(void);

/** Free @p root and everything any module allocated under it. */
void
authum_sandbox_host_root_destroy(authum_ptr root);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_HOST_SANDBOX_H */

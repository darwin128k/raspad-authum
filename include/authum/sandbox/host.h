/**
 * @file host.h
 * @brief Host side of ::authum_sandbox_t — implemented in `authum_mm` only.
 *
 * The loader creates one root per module before `start` and destroys it after
 * `stop`. Modules must not call these; they use <authum/sandbox.h>.
 */

#ifndef AUTHUM_SANDBOX_HOST_H
#define AUTHUM_SANDBOX_HOST_H

#include <authum/ptr.h>
#include <authum/sandbox/ops.h>

#ifdef __cplusplus
extern "C" {
#endif

const authum_sandbox_ops_t *
authum_sandbox_host_ops(void);

/** New empty root (owns nothing yet). */
authum_ptr
authum_sandbox_host_root_create(void);

/** Free @p root and everything any module allocated under it. */
void
authum_sandbox_host_root_destroy(authum_ptr root);

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_SANDBOX_HOST_H */

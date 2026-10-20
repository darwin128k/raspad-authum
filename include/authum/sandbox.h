/**
 * @file sandbox.h
 * @brief Module memory that cannot leak past unload.
 *
 * Every loaded module gets its own sandbox from the host
 * (::authum_host_sandbox). Anything allocated from it is freed by the host
 * after the module's `stop`, whether the module freed it or not. Freeing early
 * is still allowed and frees everything the block owns.
 *
 * A module may open nested sandboxes (::authum_sandbox_open) for shorter
 * lifetimes — a map, a connection, a request — and drop them in one call
 * (::authum_sandbox_close). Nested sandboxes are owned by their parent, so
 * forgetting to close one is not a leak either.
 *
 * @code
 * const authum_sandbox_t *sb = authum_host_sandbox(host);
 * mymod_state_t *state = authum_sandbox_alloc(sb, sizeof *state);  // zeroed
 * // … no free needed: gone after stop
 * @endcode
 */

#ifndef AUTHUM_SANDBOX_H
#define AUTHUM_SANDBOX_H

#include <authum/bool.h>
#include <authum/ptr.h>
#include <authum/sandbox/ops.h>
#include <authum/size.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct authum_sandbox
 * @brief Handle: host ops + the root block allocations go under.
 *
 * Plain value (two pointers) — copy it freely while the root is alive.
 */
struct authum_sandbox
{
    const authum_sandbox_ops_t *ops;
    authum_ptr root;
};
typedef struct authum_sandbox authum_sandbox_t;

/** @return ::authum_bool_true when @p self has ops and a root. */
authum_bool_t
authum_sandbox_is_valid(const authum_sandbox_t *self);

/** Zeroed block of @p size bytes owned by the sandbox root. Null on failure. */
authum_ptr
authum_sandbox_alloc(const authum_sandbox_t *self, authum_usize_t size);

/**
 * Zeroed block owned by @p parent (a block from this sandbox): freeing
 * @p parent frees it too. Null @p parent means the sandbox root.
 */
authum_ptr
authum_sandbox_alloc_under(const authum_sandbox_t *self, authum_ptr parent, authum_usize_t size);

/** Resize @p ptr (null @p ptr → ::authum_sandbox_alloc). Tail is not zeroed. */
authum_ptr
authum_sandbox_realloc(const authum_sandbox_t *self, authum_ptr ptr, authum_usize_t size);

/** Free @p ptr and everything it owns now. Null is ignored. */
void
authum_sandbox_free(const authum_sandbox_t *self, authum_ptr ptr);

/** Run @p fn on @p ptr right before it is freed (explicitly or at unload). */
void
authum_sandbox_set_destructor(const authum_sandbox_t *self, authum_ptr ptr,
                              authum_sandbox_destructor_fn fn);

/**
 * Open a nested sandbox owned by @p parent. Close it with
 * ::authum_sandbox_close, or let @p parent take it down.
 */
authum_bool_t
authum_sandbox_open(authum_sandbox_t *self, const authum_sandbox_t *parent);

/** Free a nested sandbox and everything in it; @p self becomes invalid. */
void
authum_sandbox_close(authum_sandbox_t *self);

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_SANDBOX_H */

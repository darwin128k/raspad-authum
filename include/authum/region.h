/**
 * @file region.h
 * @brief Module memory that cannot leak past unload.
 *
 * Every loaded module gets its own region from the host
 * (::authum_host_get_region). Anything allocated from it is freed by the host
 * after the module's `stop`, whether the module freed it or not. Freeing early
 * is still allowed and frees everything the block owns.
 *
 * A module may open nested regions (::authum_region_open) for shorter
 * lifetimes — a map, a connection, a request — and drop them in one call
 * (::authum_region_close). Nested regions are owned by their parent, so
 * forgetting to close one is not a leak either.
 *
 * @code
 * const authum_region_t *region = authum_host_get_region(host);
 * mymod_state_t *state = authum_region_alloc(region, sizeof *state);  // zeroed
 * // … no free needed: gone after stop
 * @endcode
 */

#ifndef AUTHUM_REGION_H
#define AUTHUM_REGION_H

#include <authum/bool.h>
#include <authum/region/ops.h>

/**
 * @struct authum_region
 * @brief Handle: host ops + the root block allocations go under.
 *
 * Plain value (two pointers) — copy it freely while the root is alive.
 */
struct authum_region
{
    const authum_region_ops_t *ops;
    authum_ptr root;
};
typedef struct authum_region authum_region_t;

AUTHUM_EXTERN_C_BEGIN

/** @return ::authum_bool_true when @p self has ops and a root. */
authum_bool_t
authum_region_is_valid(const authum_region_t *self);

/**
 * Zeroed block of @p size bytes owned by the region root. Null when
 * @p size is half the address space or more. Running out of memory is
 * not reported: the host ends the server process.
 */
authum_ptr
authum_region_alloc(const authum_region_t *self, authum_usize_t size);

/**
 * Zeroed block owned by @p parent (a block from this region): freeing
 * @p parent frees it too. Null @p parent means the region root.
 */
authum_ptr
authum_region_alloc_under(const authum_region_t *self, authum_ptr parent, authum_usize_t size);

/**
 * Resize @p ptr (null @p ptr → ::authum_region_alloc). Tail is not zeroed.
 * Null (and @p ptr left as it was) for the same sizes as
 * ::authum_region_alloc; the block may move, its children follow it.
 */
authum_ptr
authum_region_realloc(const authum_region_t *self, authum_ptr ptr, authum_usize_t size);

/** Free @p ptr and everything it owns now. Null is ignored. */
void
authum_region_free(const authum_region_t *self, authum_ptr ptr);

/** Run @p fn on @p ptr right before it is freed (explicitly or at unload). */
void
authum_region_set_destructor(const authum_region_t *self, authum_ptr ptr,
                             authum_region_destructor_fn fn);

/**
 * Open a nested region owned by @p parent. Close it with
 * ::authum_region_close, or let @p parent take it down.
 */
authum_bool_t
authum_region_open(authum_region_t *self, const authum_region_t *parent);

/** Free a nested region and everything in it; @p self becomes invalid. */
void
authum_region_close(authum_region_t *self);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_REGION_H */

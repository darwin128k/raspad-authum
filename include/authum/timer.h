/**
 * @file timer.h
 * @brief "In 5 seconds", "every 30 seconds" — timers for a module.
 *
 * ```c
 * authum_u32_t id = authum_timer_add(&g_host, 30.0f, 30.0f, mymod_announce, authum_null);
 * ...
 * authum_timer_remove(id);
 * ```
 *
 * Timers run on server frames (`authum.server.frame`), so a callback runs on
 * the game thread like every other handler, never in between. Time is the
 * sum of frame lengths: it keeps counting across map changes (the engine's
 * map clock starts over). Nothing to clean up: when the module unloads, its
 * frame subscription and its timer list (sandbox memory) go with it.
 */

#ifndef AUTHUM_TIMER_H
#define AUTHUM_TIMER_H

#include <authum/host.h>

AUTHUM_EXTERN_C_BEGIN

typedef void (*authum_timer_fn)(authum_ptr user);

/**
 * Run @p fn(@p user) after @p delay seconds, then every @p interval seconds
 * (0 = once). The first call subscribes this module to server frames
 * through @p host. Returns the timer id, or 0 on failure.
 */
authum_u32_t
authum_timer_add(const authum_host_t *host, authum_float_t delay, authum_float_t interval,
                 authum_timer_fn fn, authum_ptr user);

/** Stop timer @p id (safe from inside its own callback). False if there is none. */
authum_bool_t
authum_timer_remove(authum_u32_t id);

/** Stop every timer of this module. */
void
authum_timer_clear(void);

/** Timers waiting to run. */
authum_usize_t
authum_timer_count(void);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_TIMER_H */

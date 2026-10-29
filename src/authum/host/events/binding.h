/**
 * @file binding.h
 * @brief Authum callback context owned by an LH subscription.
 *
 * Allocated through Authum memory; released by LH only when dispatch no longer
 * references the subscription. It does not contain registry or list state.
 */

#ifndef AUTHUM_HOST_EVENTS_BINDING_H
#define AUTHUM_HOST_EVENTS_BINDING_H

#include <authum/event/handler.h>
#include <lh/os/module/event/cb.h>
#include <lh/os/module/event/match/cb.h>
#include <authum/host/events/binding/fields.h>

/** @brief Callback ABI bridge; use its own accessors. */
struct authum_events_binding
{
    authum_events_binding_fields(authum_event_handler_fn, authum_ptr);
};
typedef struct authum_events_binding authum_events_binding_t;

AUTHUM_EXTERN_C_BEGIN

/** @brief Level-1 callback read; self is nonnull. */
authum_event_handler_fn
authum_events_binding_get_fn(const authum_events_binding_t *self);

/** @brief Level-1 callback store; self is nonnull. */
void
authum_events_binding_set_fn(authum_events_binding_t *self, authum_event_handler_fn fn);

/** @brief Level-1 user read; self is nonnull. */
authum_ptr
authum_events_binding_get_user(const authum_events_binding_t *self);

/** @brief Level-1 user store; self is nonnull. */
void
authum_events_binding_set_user(authum_events_binding_t *self, authum_ptr user);

/** @brief Release an Authum-allocated bridge after its last active callback. */
lh_void
authum_events_binding_destroy(lh_ptr context);

/** @brief Adapt the callback result without casting incompatible function pointers. */
lh_os_module_event_result_t
authum_events_binding_call(const lh_char_t *name, lh_ptr payload, lh_ptr context);

/** @brief Compare Authum callback and user identities without dispatching either. */
lh_bool_t
authum_events_binding_matches(lh_ptr context, lh_ptr key);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_HOST_EVENTS_BINDING_H */

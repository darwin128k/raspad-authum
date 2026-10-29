/**
 * @file internal.h
 * @brief Host registry and exact module callback lookup.
 *
 * LH owns the registry, ordering and record lifetime. These helpers connect its
 * records to Authum's callback ABI and module ownership tag on the server thread.
 */

#ifndef AUTHUM_HOST_EVENTS_INTERNAL_H
#define AUTHUM_HOST_EVENTS_INTERNAL_H

#include <authum/host/events.h>
#include <lh/os/module/event/bus.h>

AUTHUM_EXTERN_C_BEGIN

/** @brief Initialize lazily and borrow the process-lifetime host registry. */
lh_os_module_event_bus_t *
authum_events_host_get_bus(void);

/** @brief Find a live exact subscription within one event; null when absent. */
lh_os_module_event_subscription_t *
authum_events_host_find(authum_ptr owner, authum_event_id_t id, authum_event_handler_fn fn,
                        authum_ptr user);

/** @brief Host implementation of the resolve operation; see the public event facade contract. */
authum_event_id_t
authum_events_host_resolve(authum_ptr owner, const authum_char_t *name);

/** @brief Host implementation of the publish_id_as operation; see the public event facade contract.
 */
authum_event_result_t
authum_events_host_publish_id_as(authum_ptr owner, authum_event_id_t id, authum_ptr payload);

/** @brief Host implementation of the publish_as operation; see the public event facade contract. */
authum_event_result_t
authum_events_host_publish_as(authum_ptr owner, const authum_char_t *name, authum_ptr payload);

/** @brief Host implementation of the subscribe_id operation; see the public event facade contract.
 */
authum_bool_t
authum_events_host_subscribe_id(authum_ptr owner, authum_event_id_t id, authum_event_handler_fn fn,
                                authum_ptr user);

/** @brief Host implementation of the unsubscribe_id operation; see the public event facade
 * contract. */
authum_bool_t
authum_events_host_unsubscribe_id(authum_ptr owner, authum_event_id_t id,
                                  authum_event_handler_fn fn, authum_ptr user);

/** @brief Host implementation of the subscribe operation; see the public event facade contract. */
authum_bool_t
authum_events_host_subscribe(authum_ptr owner, const authum_char_t *name,
                             authum_event_handler_fn fn, authum_ptr user);

/** @brief Host implementation of the unsubscribe operation; see the public event facade contract.
 */
authum_bool_t
authum_events_host_unsubscribe(authum_ptr owner, const authum_char_t *name,
                               authum_event_handler_fn fn, authum_ptr user);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_HOST_EVENTS_INTERNAL_H */

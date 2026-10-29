/**
 * @file events.h
 * @brief Host ownership facade over the indexed LH event bus.
 *
 * Modules use <authum/events.h>. The loader binds each module's
 * ::authum_events_t to these ops with the module as owner and calls
 * ::authum_events_host_drop_owner after its `stop`.
 */

#ifndef AUTHUM_HOST_EVENTS_H
#define AUTHUM_HOST_EVENTS_H

#include <authum/events/ops.h>

AUTHUM_EXTERN_C_BEGIN

const authum_events_ops_t *
authum_events_host_ops(void);

/** Publish as the host itself (`authum.*` events from engine hooks). */
authum_event_result_t
authum_events_host_publish(const authum_char_t *name, authum_ptr payload);

/** Remove every subscription made by @p owner. */
void
authum_events_host_drop_owner(authum_ptr owner);

/**
 * Whether anyone listens to @p name — lets a hook skip work (buffering a
 * user message) nobody would see.
 */
authum_bool_t
authum_events_host_has(const authum_char_t *name);

/** Live subscriptions (for diagnostics and tests). */
authum_usize_t
authum_events_host_count(void);

/** Remove subscriptions; retain registry names and IDs for the host lifetime. */
void
authum_events_host_clear(void);

/** @brief Resolve a host-lifetime ID, or zero on invalid name/allocation failure. */
authum_event_id_t
authum_events_host_get_id(const authum_char_t *name);

/** @brief Resolve only a zero cache; failures leave zero and may be retried. */
authum_event_id_t
authum_events_host_get_cached_id(authum_event_id_t *cache, const authum_char_t *name);

/** @brief Publish a previously resolved ID without string lookup. */
authum_event_result_t
authum_events_host_publish_id(authum_event_id_t id, authum_ptr payload);

/** @brief Whether a resolved event has a live subscription. */
authum_bool_t
authum_events_host_has_id(authum_event_id_t id);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_HOST_EVENTS_H */

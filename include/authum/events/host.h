/**
 * @file host.h
 * @brief Host side of the event bus — implemented in `authum` only.
 *
 * Modules use <authum/events.h>. The loader binds each module's
 * ::authum_events_t to these ops with the module as owner and calls
 * ::authum_events_host_drop_owner after its `stop`.
 */

#ifndef AUTHUM_EVENTS_HOST_H
#define AUTHUM_EVENTS_HOST_H

#include <authum/char.h>
#include <authum/event/result.h>
#include <authum/events/ops.h>
#include <authum/ptr.h>
#include <authum/size.h>

#ifdef __cplusplus
extern "C"
{
#endif

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

    /** Remove all subscriptions (host shutdown). */
    void
    authum_events_host_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_EVENTS_HOST_H */

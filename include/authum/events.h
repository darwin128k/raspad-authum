/**
 * @file events.h
 * @brief Event bus between Authum and its modules (forwards + queries).
 *
 * One mechanism for all module-to-module talk:
 *
 * - **Forward** — the publisher announces something (`authum.client.connect`,
 *   `qproto.client.authorized`); every subscriber sees it in subscription
 *   order. A handler may return ::authum_event_result_stop to end the chain.
 * - **Query** — the publisher fills the in-fields of a payload, publishes, and
 *   the module that owns the answer fills the out-fields and returns
 *   ::authum_event_result_handled. ::authum_event_result_continue from
 *   ::authum_events_publish means nobody answered.
 *
 * Modules never hold pointers into each other: no dlsym, no shared function
 * tables. A module that unloads simply stops answering — the host drops its
 * subscriptions after `stop`.
 *
 * Names are dotted, owner first: `authum.*` for the host,
 * `<module>.*` for a module (`qproto.client.query`). Payload structs are
 * declared by the owner in its public header and start with
 * `authum_usize_t size` (= sizeof the struct the publisher built) so they can
 * grow without breaking older subscribers.
 */

#ifndef AUTHUM_EVENTS_H
#define AUTHUM_EVENTS_H

#include <authum/bool.h>
#include <authum/char.h>
#include <authum/event/handler.h>
#include <authum/event/result.h>
#include <authum/events/ops.h>
#include <authum/ptr.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @struct authum_events
     * @brief Per-module handle: host ops + who is calling.
     */
    struct authum_events
    {
        const authum_events_ops_t *ops;
        authum_ptr owner;
    };
    typedef struct authum_events authum_events_t;

    /**
     * Call @p fn for every event named @p name until unsubscribed or unloaded.
     * The same (@p name, @p fn, @p user) triple is registered once.
     */
    authum_bool_t
    authum_events_subscribe(const authum_events_t *self, const authum_char_t *name,
                            authum_event_handler_fn fn, authum_ptr user);

    authum_bool_t
    authum_events_unsubscribe(const authum_events_t *self, const authum_char_t *name,
                              authum_event_handler_fn fn, authum_ptr user);

    /**
     * Deliver @p payload to subscribers of @p name.
     * @return Strongest result any handler returned (continue when none).
     */
    authum_event_result_t
    authum_events_publish(const authum_events_t *self, const authum_char_t *name,
                          authum_ptr payload);

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_EVENTS_H */

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
 *   ::authum_events_query means nobody answered.
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

#include <authum/events/ops.h>
#include <authum/event/name/limits.h>
#include <authum/events/fields.h>

/**
 * @struct authum_events
 * @brief Per-module handle: host ops + who is calling.
 */
struct authum_events
{
    authum_events_fields(const authum_events_ops_t *, authum_ptr);
};
typedef struct authum_events authum_events_t;

AUTHUM_EXTERN_C_BEGIN

/** @brief Level-1 read of ops; self must be nonnull. */
const authum_events_ops_t *
authum_events_get_ops(const authum_events_t *self);

/** @brief Level-1 store of ops; no validation or ownership transfer. */
void
authum_events_set_ops(authum_events_t *self, const authum_events_ops_t *value);

/** @brief Level-1 read of owner; self must be nonnull. */
authum_ptr
authum_events_get_owner(const authum_events_t *self);

/** @brief Level-1 store of owner; no validation or ownership transfer. */
void
authum_events_set_owner(authum_events_t *self, authum_ptr value);

/**
 * @brief The names this module owns: `scores.` for the module `scores`.
 *
 * The host sets it when the module starts. A forward must start with it.
 */
const authum_char_t *
authum_events_get_prefix(const authum_events_t *self);

/**
 * @brief Set the prefix of the names this module owns.
 */
void
authum_events_set_prefix(authum_events_t *self, const authum_char_t *prefix);

/** @brief Subscribe once per (owner, name, fn, user); false on invalid name or allocation failure.
 */
authum_bool_t
authum_events_subscribe(const authum_events_t *self, const authum_char_t *name,
                        authum_event_handler_fn fn, authum_ptr user);

/** @brief Remove an exact subscription; false when absent. Takes effect immediately. */
authum_bool_t
authum_events_unsubscribe(const authum_events_t *self, const authum_char_t *name,
                          authum_event_handler_fn fn, authum_ptr user);

/**
 * @brief Announce an event of your own and publish it; the name must be yours.
 *
 * The name must start with the module's own prefix (`scores.` for the module
 * `scores`): a module never publishes under another module's name. A module
 * that asks another module something uses ::authum_events_query.
 *
 * @return The chain's result; ::authum_event_result_continue when the name is
 *         not the module's own.
 */
authum_event_result_t
authum_events_forward(const authum_events_t *self, const authum_char_t *name, authum_ptr payload);

/**
 * @brief Ask the module that owns @p name; it answers by filling the out-fields.
 *
 * The name belongs to another module: that is the point of a query. The owner
 * of the name fills the out-fields and returns ::authum_event_result_handled.
 *
 * @return The chain's result; ::authum_event_result_continue when nobody
 *         answered.
 */
authum_event_result_t
authum_events_query(const authum_events_t *self, const authum_char_t *name, authum_ptr payload);

/** @brief Intern a nonempty name of at most 63 characters; zero on failure. */
authum_event_id_t
authum_events_get_id(const authum_events_t *self, const authum_char_t *name);

/** @brief Subscribe by resolved ID, preserving the same deduplication and ownership rules. */
authum_bool_t
authum_events_subscribe_id(const authum_events_t *self, authum_event_id_t id,
                           authum_event_handler_fn fn, authum_ptr user);

/** @brief Remove an exact numeric subscription; false if absent or unsupported. */
authum_bool_t
authum_events_unsubscribe_id(const authum_events_t *self, authum_event_id_t id,
                             authum_event_handler_fn fn, authum_ptr user);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_EVENTS_H */

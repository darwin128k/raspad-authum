/**
 * @file ops.h
 * @brief The host's event dispatch table behind ::authum_events_t.
 *
 * Modules call the `authum_events_*` functions in <authum/events.h>, never these
 * pointers. Names and IDs are resolved by the host registry; the table is append-only
 * (see ::AUTHUM_MODULE_ABI).
 */

#ifndef AUTHUM_EVENTS_OPS_H
#define AUTHUM_EVENTS_OPS_H

#include <authum/events/ops/fields.h>

/** @brief Append-only module ABI; access through the events facade. */
struct authum_events_ops
{
    authum_events_ops_fields();
};
typedef struct authum_events_ops authum_events_ops_t;

AUTHUM_EXTERN_C_BEGIN

/** @brief Level-1 read of subscribe; self must be nonnull and reach this field. */
authum_events_subscribe_cb
authum_events_ops_get_subscribe(const authum_events_ops_t *self);

/** @brief Level-1 read of unsubscribe; self must be nonnull and reach this field. */
authum_events_unsubscribe_cb
authum_events_ops_get_unsubscribe(const authum_events_ops_t *self);

/** @brief Level-1 read of publish; self must be nonnull and reach this field. */
authum_events_publish_cb
authum_events_ops_get_publish(const authum_events_ops_t *self);

/** @brief Level-1 read of resolve; self must be nonnull and reach this field. */
authum_events_resolve_cb
authum_events_ops_get_resolve(const authum_events_ops_t *self);

/** @brief Level-1 read of subscribe_id; self must be nonnull and reach this field. */
authum_events_subscribe_id_cb
authum_events_ops_get_subscribe_id(const authum_events_ops_t *self);

/** @brief Level-1 read of unsubscribe_id; self must be nonnull and reach this field. */
authum_events_unsubscribe_id_cb
authum_events_ops_get_unsubscribe_id(const authum_events_ops_t *self);

/** @brief Whether the table provides subscribe; null is unavailable. */
authum_bool_t
authum_events_ops_has_subscribe(const authum_events_ops_t *self);

/** @brief Whether the table provides unsubscribe; null is unavailable. */
authum_bool_t
authum_events_ops_has_unsubscribe(const authum_events_ops_t *self);

/** @brief Whether the table provides publish; null is unavailable. */
authum_bool_t
authum_events_ops_has_publish(const authum_events_ops_t *self);

/** @brief Whether the table provides resolve; null is unavailable. */
authum_bool_t
authum_events_ops_has_resolve(const authum_events_ops_t *self);

/** @brief Whether the table provides subscribe_id; null is unavailable. */
authum_bool_t
authum_events_ops_has_subscribe_id(const authum_events_ops_t *self);

/** @brief Whether the table provides unsubscribe_id; null is unavailable. */
authum_bool_t
authum_events_ops_has_unsubscribe_id(const authum_events_ops_t *self);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_EVENTS_OPS_H */

/**
 * @file ops.h
 * @brief Sized host event dispatch table, preserving the original three operations.
 *
 * Old modules retain the string operations at their original offsets. New
 * modules check table size before numeric operations, reporting unavailable
 * on older hosts. Names and IDs are resolved by the host registry.
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

/** @brief Level-1 read of size; self must be nonnull and reach this field. */
authum_usize_t
authum_events_ops_get_size(const authum_events_ops_t *self);

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

/** @brief Level-1 read of publish_id; self must be nonnull and reach this field. */
authum_events_publish_id_cb
authum_events_ops_get_publish_id(const authum_events_ops_t *self);

/** @brief Level-1 size store, for constructing a sized ABI table; no validation. */
void
authum_events_ops_set_size(authum_events_ops_t *self, authum_usize_t size);

/**
 * @brief Whether the host's table is long enough to hold the bytes up to @p end (level 2).
 *
 * A table built by an older host stops after the members it knew. @p end is the
 * offset just past a member: its offset plus its size.
 *
 * @param self The table, or ::authum_null (which reaches nothing).
 * @param end  Offset just past the member asked about.
 */
authum_bool_t
authum_events_ops_reaches(const authum_events_ops_t *self, authum_usize_t end);

/** @brief Whether the sized ABI table provides subscribe; null is unavailable. */
authum_bool_t
authum_events_ops_has_subscribe(const authum_events_ops_t *self);

/** @brief Whether the sized ABI table provides unsubscribe; null is unavailable. */
authum_bool_t
authum_events_ops_has_unsubscribe(const authum_events_ops_t *self);

/** @brief Whether the sized ABI table provides publish; null is unavailable. */
authum_bool_t
authum_events_ops_has_publish(const authum_events_ops_t *self);

/** @brief Whether the sized ABI table provides resolve; null is unavailable. */
authum_bool_t
authum_events_ops_has_resolve(const authum_events_ops_t *self);

/** @brief Whether the sized ABI table provides subscribe_id; null is unavailable. */
authum_bool_t
authum_events_ops_has_subscribe_id(const authum_events_ops_t *self);

/** @brief Whether the sized ABI table provides unsubscribe_id; null is unavailable. */
authum_bool_t
authum_events_ops_has_unsubscribe_id(const authum_events_ops_t *self);

/** @brief Whether the sized ABI table provides publish_id; null is unavailable. */
authum_bool_t
authum_events_ops_has_publish_id(const authum_events_ops_t *self);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_EVENTS_OPS_H */

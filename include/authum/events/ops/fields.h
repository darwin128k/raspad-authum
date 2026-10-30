/**
 * @file fields.h
 * @brief Append-only event operations table members.
 *
 * Append-only event operations table members.
 */

#ifndef AUTHUM_EVENTS_OPS_FIELDS_H
#define AUTHUM_EVENTS_OPS_FIELDS_H

#include <authum/events/subscribe/cb.h>
#include <authum/events/unsubscribe/cb.h>
#include <authum/events/publish/cb.h>
#include <authum/events/resolve/cb.h>
#include <authum/events/subscribe_id/cb.h>
#include <authum/events/unsubscribe_id/cb.h>

/** @brief ABI member order; append operations only. */
#define authum_events_ops_fields()                                                                 \
    authum_events_subscribe_cb subscribe;                                                          \
    authum_events_unsubscribe_cb unsubscribe;                                                      \
    authum_events_publish_cb publish;                                                              \
    authum_events_resolve_cb resolve;                                                              \
    authum_events_subscribe_id_cb subscribe_id;                                                    \
    authum_events_unsubscribe_id_cb unsubscribe_id

#endif /* AUTHUM_EVENTS_OPS_FIELDS_H */

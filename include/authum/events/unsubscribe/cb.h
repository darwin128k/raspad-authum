/**
 * @file cb.h
 * @brief Pointer to the host unsubscribe operation.
 *
 * Pointer to the host unsubscribe operation.
 */

#ifndef AUTHUM_EVENTS_UNSUBSCRIBE_CB_H
#define AUTHUM_EVENTS_UNSUBSCRIBE_CB_H

#include <authum/events/unsubscribe/fn.h>

/** @brief Host operation pointer; null means unavailable. */
typedef authum_events_unsubscribe_fn *authum_events_unsubscribe_cb;

#endif /* AUTHUM_EVENTS_UNSUBSCRIBE_CB_H */

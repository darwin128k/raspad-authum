/**
 * @file cb.h
 * @brief Pointer to the host publish operation.
 *
 * Pointer to the host publish operation.
 */

#ifndef AUTHUM_EVENTS_PUBLISH_CB_H
#define AUTHUM_EVENTS_PUBLISH_CB_H

#include <authum/events/publish/fn.h>

/** @brief Host operation pointer; null means unavailable. */
typedef authum_events_publish_fn *authum_events_publish_cb;

#endif /* AUTHUM_EVENTS_PUBLISH_CB_H */

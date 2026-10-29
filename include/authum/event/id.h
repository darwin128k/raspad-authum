/**
 * @file id.h
 * @brief Host-local numeric event identity; zero is unavailable.
 *
 * Host-local numeric event identity; zero is unavailable.
 */

#ifndef AUTHUM_EVENT_ID_H
#define AUTHUM_EVENT_ID_H

#include <authum/size.h>

/**
 * @brief Stable ID within the current host lifetime; never persist it to disk.
 * Resolve the public event name again after restarting the host. Clearing
 * subscriptions and unloading a module do not invalidate IDs.
 */
typedef authum_usize_t authum_event_id_t;

#endif /* AUTHUM_EVENT_ID_H */

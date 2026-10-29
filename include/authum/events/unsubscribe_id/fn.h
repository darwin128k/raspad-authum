/**
 * @file fn.h
 * @brief Host dispatch function type for unsubscribe_id.
 *
 * Host dispatch function type for unsubscribe_id.
 */

#ifndef AUTHUM_EVENTS_UNSUBSCRIBE_ID_FN_H
#define AUTHUM_EVENTS_UNSUBSCRIBE_ID_FN_H

#include <authum/event/id.h>
#include <authum/event/handler.h>
#include <authum/bool.h>

AUTHUM_EXTERN_C_BEGIN

/** @brief Host ABI operation; use the public events facade rather than calling directly. */
typedef authum_bool_t(authum_events_unsubscribe_id_fn)(authum_ptr owner, authum_event_id_t id,
                                                       authum_event_handler_fn fn, authum_ptr user);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_EVENTS_UNSUBSCRIBE_ID_FN_H */

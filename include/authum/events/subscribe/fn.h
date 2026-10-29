/**
 * @file fn.h
 * @brief Host dispatch function type for subscribe.
 *
 * Host dispatch function type for subscribe.
 */

#ifndef AUTHUM_EVENTS_SUBSCRIBE_FN_H
#define AUTHUM_EVENTS_SUBSCRIBE_FN_H

#include <authum/event/handler.h>
#include <authum/bool.h>

AUTHUM_EXTERN_C_BEGIN

/** @brief Host ABI operation; use the public events facade rather than calling directly. */
typedef authum_bool_t(authum_events_subscribe_fn)(authum_ptr owner, const authum_char_t *name,
                                                  authum_event_handler_fn fn, authum_ptr user);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_EVENTS_SUBSCRIBE_FN_H */

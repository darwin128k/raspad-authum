/**
 * @file fn.h
 * @brief Host dispatch function type for publish_id.
 *
 * Host dispatch function type for publish_id.
 */

#ifndef AUTHUM_EVENTS_PUBLISH_ID_FN_H
#define AUTHUM_EVENTS_PUBLISH_ID_FN_H

#include <authum/event/id.h>
#include <authum/event/result.h>
#include <authum/char.h>
#include <authum/ptr.h>
#include <authum/extern.h>

AUTHUM_EXTERN_C_BEGIN

/** @brief Host ABI operation; use the public events facade rather than calling directly. */
typedef authum_event_result_t(authum_events_publish_id_fn)(authum_ptr owner, authum_event_id_t id,
                                                           authum_ptr payload);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_EVENTS_PUBLISH_ID_FN_H */

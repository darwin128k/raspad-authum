/**
 * @file fn.h
 * @brief Host dispatch function type for resolve.
 *
 * Host dispatch function type for resolve.
 */

#ifndef AUTHUM_EVENTS_RESOLVE_FN_H
#define AUTHUM_EVENTS_RESOLVE_FN_H

#include <authum/event/id.h>
#include <authum/char.h>
#include <authum/ptr.h>
#include <authum/extern.h>

AUTHUM_EXTERN_C_BEGIN

/** @brief Host ABI operation; use the public events facade rather than calling directly. */
typedef authum_event_id_t(authum_events_resolve_fn)(authum_ptr owner, const authum_char_t *name);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_EVENTS_RESOLVE_FN_H */

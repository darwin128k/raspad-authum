/**
 * @file handler.h
 * @brief Event handler signature.
 */

#ifndef AUTHUM_EVENT_HANDLER_H
#define AUTHUM_EVENT_HANDLER_H

#include <authum/char.h>
#include <authum/event/result.h>
#include <authum/extern.h>
#include <authum/ptr.h>

AUTHUM_EXTERN_C_BEGIN

/**
 * @param name    Event name as published (`authum.client.connect`, …).
 * @param payload Publisher-defined struct; may carry out-fields for queries.
 * @param user    The pointer given at subscribe time.
 */
typedef authum_event_result_t (*authum_event_handler_fn)(const authum_char_t *name,
                                                         authum_ptr payload, authum_ptr user);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_EVENT_HANDLER_H */

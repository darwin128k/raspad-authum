/**
 * @file fields.h
 * @brief Members of an Authum callback ABI bridge.
 *
 * The bridge has no subscription list or dispatch state; LH owns those.
 */

#ifndef AUTHUM_HOST_EVENTS_BINDING_FIELDS_H
#define AUTHUM_HOST_EVENTS_BINDING_FIELDS_H

/** @brief Declare the callback pointer and its borrowed user pointer. */
#define authum_events_binding_fields(callback_type, user_type)                                     \
    callback_type fn;                                                                              \
    user_type user

#endif /* AUTHUM_HOST_EVENTS_BINDING_FIELDS_H */

/**
 * @file result.h
 * @brief What an event handler tells the bus.
 */

#ifndef AUTHUM_EVENT_RESULT_H
#define AUTHUM_EVENT_RESULT_H

/**
 * Ordered by strength: ::authum_events_forward and ::authum_events_query return
 * the strongest result any handler gave.
 */
enum authum_event_result
{
    /** Not mine / nothing to say — keep calling the next handler. */
    authum_event_result_continue = 0,
    /** I acted (or answered a query) — still call the rest. */
    authum_event_result_handled = 1,
    /** I acted and nobody after me should see this event. */
    authum_event_result_stop = 2
};
typedef enum authum_event_result authum_event_result_t;

#endif /* AUTHUM_EVENT_RESULT_H */

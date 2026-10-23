/**
 * @file ops.h
 * @brief Host ops table behind ::authum_events_t (filled by `authum`).
 *
 * Modules call the `authum_events_*` functions in <authum/events.h>, never
 * these pointers. @p owner identifies the subscribing module so the host can
 * drop all of its subscriptions at unload.
 */

#ifndef AUTHUM_EVENTS_OPS_H
#define AUTHUM_EVENTS_OPS_H

#include <authum/bool.h>
#include <authum/char.h>
#include <authum/abi.h>
#include <authum/event/handler.h>
#include <authum/event/result.h>
#include <authum/ptr.h>

#ifdef __cplusplus
extern "C"
{
#endif

    struct authum_events_ops
    {
        /** sizeof this table as built by the host — see <authum/abi.h>. */
        authum_usize_t size;
        authum_bool_t (*subscribe)(authum_ptr owner, const authum_char_t *name,
                                   authum_event_handler_fn fn, authum_ptr user);
        authum_bool_t (*unsubscribe)(authum_ptr owner, const authum_char_t *name,
                                     authum_event_handler_fn fn, authum_ptr user);
        authum_event_result_t (*publish)(authum_ptr owner, const authum_char_t *name,
                                         authum_ptr payload);
    };
    typedef struct authum_events_ops authum_events_ops_t;

#ifdef __cplusplus
}
#endif

#endif /* AUTHUM_EVENTS_OPS_H */

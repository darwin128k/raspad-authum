/**
 * @file private.h
 * @brief Private to `timer.c`: a module timer, an `lh_timer_t` with the module's id,
 *        callback and value.
 *
 * Not installed: modules never see these.
 */

#ifndef AUTHUM_API_TIMER_PRIVATE_H
#define AUTHUM_API_TIMER_PRIVATE_H

#include <authum/timer.h>
#include <lh/core/timer.h>

/** @brief One module timer: lh's timer, and what the module gave for it. */
typedef struct authum_timer_entry
{
    lh_timer_t timer;                /**< lh's timer, armed on the module's group */
    struct authum_timer_entry *next; /**< the next live timer of the module */
    authum_u32_t id;                 /**< what ::authum_timer_add returned */
    authum_timer_fn fn;              /**< the module's callback */
    authum_ptr user;                 /**< the value @c fn is given */
} authum_timer_entry_t;

AUTHUM_EXTERN_C_BEGIN

/**
 * @brief Microseconds in @p seconds, rounded; 0 for none or a negative time.
 */
authum_u64_t
authum_timer_us(authum_float_t seconds);

/**
 * @brief lh's tick for @p us microseconds: milliseconds, wrapping as lh's tick does.
 */
lh_tick_t
authum_timer_tick(authum_u64_t us);

/**
 * @brief Unlink @p entry from the module's live timers, stop it and free it.
 *
 * Safe from inside any timer callback, its own included: lh's handler touches no
 * timer after its callback returns.
 */
void
authum_timer_free(authum_timer_entry_t *entry);

/**
 * @brief lh's callback for every module timer: runs the module's one, freeing a
 *        one-shot timer first so that the callback may add or remove any timer.
 */
lh_void
authum_timer_fire(lh_timer_t *timer, lh_ptr context);

/**
 * @brief The server-frame handler: advance the module's clock and fire what is due.
 */
authum_event_result_t
authum_timer_on_frame(const authum_char_t *name, authum_ptr payload, authum_ptr user);

/**
 * @brief Lh's timer (level 1).
 *
 * @param self The timer entry; must not be ::authum_null (checked).
 */
const lh_timer_t *
authum_timer_entry_get_timer(const authum_timer_entry_t *self);

/**
 * @brief Lh's timer, to change in place (level 1).
 *
 * @param self The timer entry; must not be ::authum_null (checked).
 * @return The member itself, never ::authum_null.
 */
lh_timer_t *
authum_timer_entry_get_timer_mut(authum_timer_entry_t *self);

/**
 * @brief The next live timer of the module (level 1).
 *
 * @param self The timer entry; must not be ::authum_null (checked).
 */
struct authum_timer_entry *
authum_timer_entry_get_next(const authum_timer_entry_t *self);

/**
 * @brief Store the next live timer of the module (level 1).
 *
 * A level-1 setter: the value is stored as given and is not validated.
 *
 * @param self The timer entry; must not be ::authum_null (checked).
 * @param next The new value.
 */
void
authum_timer_entry_set_next(authum_timer_entry_t *self, struct authum_timer_entry *next);

/**
 * @brief The id authum_timer_add returned (level 1).
 *
 * @param self The timer entry; must not be ::authum_null (checked).
 */
authum_u32_t
authum_timer_entry_get_id(const authum_timer_entry_t *self);

/**
 * @brief Store the id authum_timer_add returned (level 1).
 *
 * A level-1 setter: the value is stored as given and is not validated.
 *
 * @param self The timer entry; must not be ::authum_null (checked).
 * @param id The new value.
 */
void
authum_timer_entry_set_id(authum_timer_entry_t *self, authum_u32_t id);

/**
 * @brief The module's callback (level 1).
 *
 * @param self The timer entry; must not be ::authum_null (checked).
 */
authum_timer_fn
authum_timer_entry_get_fn(const authum_timer_entry_t *self);

/**
 * @brief Store the module's callback (level 1).
 *
 * A level-1 setter: the value is stored as given and is not validated.
 *
 * @param self The timer entry; must not be ::authum_null (checked).
 * @param fn The new value.
 */
void
authum_timer_entry_set_fn(authum_timer_entry_t *self, authum_timer_fn fn);

/**
 * @brief The value the callback is given (level 1).
 *
 * @param self The timer entry; must not be ::authum_null (checked).
 */
authum_ptr
authum_timer_entry_get_user(const authum_timer_entry_t *self);

/**
 * @brief Store the value the callback is given (level 1).
 *
 * A level-1 setter: the value is stored as given and is not validated.
 *
 * @param self The timer entry; must not be ::authum_null (checked).
 * @param user The new value.
 */
void
authum_timer_entry_set_user(authum_timer_entry_t *self, authum_ptr user);

AUTHUM_EXTERN_C_END

#endif /* AUTHUM_API_TIMER_PRIVATE_H */

/**
 * @file timer.c
 * @brief Module timers on server frames over lh's timer group — see <authum/timer.h>.
 *
 * authum_client is linked into each module, so these globals are per module. Each
 * timer is an `lh_timer_t` in region memory, armed on the module's group; the group
 * is driven by the sum of server frame lengths, in milliseconds. lh keeps the phase
 * of a repeating timer and skips missed periods instead of bursting.
 */

#include <authum/api/timer/private.h>
#include <authum/event/server.h>
#include <authum/unused.h>
#include <lh/core/assert/runtime.h>

lh_timer_group_t authum_timers_group;
/** The live timers, newest first: what remove, clear and count walk. */
authum_timer_entry_t *authum_timers_head;
/** The module's region, which the timers live in. */
authum_region_t authum_timers_region;
authum_bool_t authum_timers_ready;
/** Microseconds of server frames since the first timer was added. */
authum_u64_t authum_timers_clock;
authum_u32_t authum_timers_next_id = 1u;
const lh_timer_t *
authum_timer_entry_get_timer(const authum_timer_entry_t *self)
{
    lh_assert_runtime_ref(self);
    return lh_addr_of(self->timer);
}

lh_timer_t *
authum_timer_entry_get_timer_mut(authum_timer_entry_t *self)
{
    lh_assert_runtime_ref(self);
    return lh_addr_of(self->timer);
}

struct authum_timer_entry *
authum_timer_entry_get_next(const authum_timer_entry_t *self)
{
    lh_assert_runtime_ref(self);
    return self->next;
}

void
authum_timer_entry_set_next(authum_timer_entry_t *self, struct authum_timer_entry *next)
{
    lh_assert_runtime_ref(self);
    self->next = next;
}

authum_u32_t
authum_timer_entry_get_id(const authum_timer_entry_t *self)
{
    lh_assert_runtime_ref(self);
    return self->id;
}

void
authum_timer_entry_set_id(authum_timer_entry_t *self, authum_u32_t id)
{
    lh_assert_runtime_ref(self);
    self->id = id;
}

authum_timer_fn
authum_timer_entry_get_fn(const authum_timer_entry_t *self)
{
    lh_assert_runtime_ref(self);
    return self->fn;
}

void
authum_timer_entry_set_fn(authum_timer_entry_t *self, authum_timer_fn fn)
{
    lh_assert_runtime_ref(self);
    self->fn = fn;
}

authum_ptr
authum_timer_entry_get_user(const authum_timer_entry_t *self)
{
    lh_assert_runtime_ref(self);
    return self->user;
}

void
authum_timer_entry_set_user(authum_timer_entry_t *self, authum_ptr user)
{
    lh_assert_runtime_ref(self);
    self->user = user;
}

authum_u64_t
authum_timer_us(authum_float_t seconds)
{
    /* Rounded: 0.01f * 1e6 is 9999.9998, and truncating would lose 1 µs a frame. */
    return lh_math_gt(seconds, 0.0f)
               ? (authum_u64_t)(lh_math_add(lh_math_mul(seconds, 1000000.0f), 0.5f))
               : 0u;
}

lh_tick_t
authum_timer_tick(authum_u64_t us)
{
    return lh_cast_static(lh_tick_t, us / 1000u);
}

void
authum_timer_free(authum_timer_entry_t *entry)
{
    if (lh_math_eq(authum_timers_head, entry))
    {
        authum_timers_head = authum_timer_entry_get_next(entry);
    }
    else
    {
        authum_timer_entry_t *prev = authum_timers_head;

        while (!authum_null_eq(prev) && lh_math_ne(authum_timer_entry_get_next(prev), entry))
        {
            prev = authum_timer_entry_get_next(prev);
        }
        if (!authum_null_eq(prev))
        {
            authum_timer_entry_set_next(prev, authum_timer_entry_get_next(entry));
        }
    }
    lh_timer_deinit(authum_timer_entry_get_timer_mut(entry));
    authum_region_free(lh_addr_of(authum_timers_region), entry);
}

authum_event_result_t
authum_timer_on_frame(const authum_char_t *name, authum_ptr payload, authum_ptr user)
{
    const authum_event_server_t *ev = lh_cast_reinterpret(const authum_event_server_t *, payload);

    authum_unused(name);
    authum_unused(user);
    if (authum_null_eq(ev))
    {
        return authum_event_result_continue;
    }
    authum_timers_clock = lh_math_add(authum_timers_clock, authum_timer_us(ev->frame_time));
    lh_timer_group_handler(lh_addr_of(authum_timers_group), authum_timer_tick(authum_timers_clock));
    return authum_event_result_continue;
}

authum_usize_t
authum_timer_get_count(void)
{
    authum_usize_t count = 0u;

    for (const authum_timer_entry_t *entry = authum_timers_head; !authum_null_eq(entry);
         entry = authum_timer_entry_get_next(entry))
    {
        count = lh_math_add_one(count);
    }
    return count;
}

lh_void
authum_timer_fire(lh_timer_t *timer, lh_ptr context)
{
    authum_timer_entry_t *entry = context;
    const authum_timer_fn fn = authum_timer_entry_get_fn(entry);
    const authum_ptr user = authum_timer_entry_get_user(entry);

    authum_unused(timer);
    /* lh already stopped a one-shot timer: it is done, and the callback may remove
       or add any timer, so nothing of the entry is touched after it. */
    if (!lh_timer_get_repeat(authum_timer_entry_get_timer_mut(entry)))
    {
        authum_timer_free(entry);
    }
    fn(user);
}

authum_bool_t
authum_timer_remove(authum_u32_t id)
{
    for (authum_timer_entry_t *entry = authum_timers_head; !authum_null_eq(entry);
         entry = authum_timer_entry_get_next(entry))
    {
        if (lh_math_eq(authum_timer_entry_get_id(entry), id))
        {
            authum_timer_free(entry);
            return authum_bool_true;
        }
    }
    return authum_bool_false;
}

void
authum_timer_clear(void)
{
    while (!authum_null_eq(authum_timers_head))
    {
        authum_timer_free(authum_timers_head);
    }
}

authum_u32_t
authum_timer_add(const authum_host_t *host, authum_float_t delay, authum_float_t interval,
                 authum_timer_fn fn, authum_ptr user)
{
    authum_timer_entry_t *entry;
    const authum_u64_t period = authum_timer_us(interval);

    if (lh_math_eq(fn, 0))
    {
        return 0u;
    }
    if (!authum_timers_ready)
    {
        if (authum_null_eq(host) || !authum_region_is_valid(authum_host_get_region(host)) ||
            !authum_host_subscribe(host, AUTHUM_EVENT_SERVER_FRAME, authum_timer_on_frame,
                                   authum_null))
        {
            return 0u;
        }
        authum_timers_region = lh_ptr_deref(authum_host_get_region(host));
        lh_timer_group_init(lh_addr_of(authum_timers_group));
        authum_timers_ready = authum_bool_true;
    }
    entry = authum_region_alloc(lh_addr_of(authum_timers_region), sizeof lh_ptr_deref(entry));
    if (authum_null_eq(entry))
    {
        return 0u;
    }
    authum_timer_entry_set_id(entry, authum_timers_next_id++);
    if (lh_math_eq(authum_timers_next_id, 0u))
    {
        authum_timers_next_id = 1u;
    }
    authum_timer_entry_set_fn(entry, fn);
    authum_timer_entry_set_user(entry, user);
    authum_timer_entry_set_next(entry, authum_timers_head);
    authum_timers_head = entry;
    lh_timer_init(authum_timer_entry_get_timer_mut(entry));
    /* A period under a millisecond still repeats: lh's period is at least one tick. */
    lh_timer_start_after(lh_addr_of(authum_timers_group), authum_timer_entry_get_timer_mut(entry),
                         authum_timer_tick(authum_timer_us(delay)),
                         lh_math_max(authum_timer_tick(period), 1u),
                         lh_math_ne(period, 0u) ? lh_bool_true : lh_bool_false, 0u,
                         authum_timer_fire, entry, authum_timer_tick(authum_timers_clock));
    return authum_timer_entry_get_id(entry);
}

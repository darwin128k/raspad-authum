/**
 * @file timer.c
 * @brief Module timers on server frames — see <authum/timer.h>.
 *
 * authum_client is linked into each module, so these globals are per module.
 */

#include <authum/timer.h>

#include <authum/array.h>
#include <authum/event/server.h>
#include <authum/events.h>

typedef struct authum_timer
{
    authum_u32_t id;
    /** Clock (µs) at which it runs next. */
    authum_u64_t due;
    /** Repeat period in µs; 0 = once. */
    authum_u64_t interval;
    authum_timer_fn fn;
    authum_ptr user;
    authum_bool_t dead;
} authum_timer_t;

authum_array_t authum_timers;
authum_bool_t authum_timers_ready;
/** Microseconds of server frames since the first timer was added. */
authum_u64_t authum_timers_clock;
authum_u32_t authum_timers_next_id = 1u;
/** Inside the frame handler: removals only mark, the sweep comes after. */
authum_bool_t authum_timers_running;

authum_u64_t
authum_timer_us(authum_float_t seconds)
{
    /* Rounded: 0.01f * 1e6 is 9999.9998, and truncating would lose 1 µs a frame. */
    return seconds > 0.0f ? (authum_u64_t)(seconds * 1000000.0f + 0.5f) : 0u;
}

void
authum_timer_sweep(void)
{
    authum_usize_t i = authum_array_size(&authum_timers);

    while (i > 0u)
    {
        --i;
        if (((authum_timer_t *)authum_array_at(&authum_timers, i))->dead)
        {
            authum_array_remove_at(&authum_timers, i);
        }
    }
}

authum_event_result_t
authum_timer_on_frame(const authum_char_t *name, authum_ptr payload, authum_ptr user)
{
    const authum_event_server_t *ev = (const authum_event_server_t *)payload;
    authum_usize_t i;

    (void)name;
    (void)user;
    if (authum_null_eq(ev) || ev->size < sizeof *ev)
    {
        return authum_event_result_continue;
    }
    authum_timers_clock += authum_timer_us(ev->frame_time);
    authum_timers_running = authum_bool_true;
    /* By index: a callback may add timers and the array may move. */
    for (i = 0u; i < authum_array_size(&authum_timers); ++i)
    {
        authum_timer_t *timer = (authum_timer_t *)authum_array_at(&authum_timers, i);
        authum_timer_fn fn;
        authum_ptr fn_user;

        if (timer->dead || timer->due > authum_timers_clock)
        {
            continue;
        }
        fn = timer->fn;
        fn_user = timer->user;
        if (timer->interval == 0u)
        {
            timer->dead = authum_bool_true;
        }
        else
        {
            timer->due += timer->interval;
            /* Fell behind (a long frame): no burst of catch-up calls. */
            if (timer->due <= authum_timers_clock)
            {
                timer->due = authum_timers_clock + timer->interval;
            }
        }
        fn(fn_user);
    }
    authum_timers_running = authum_bool_false;
    authum_timer_sweep();
    return authum_event_result_continue;
}

authum_u32_t
authum_timer_add(const authum_host_t *host, authum_float_t delay, authum_float_t interval,
                 authum_timer_fn fn, authum_ptr user)
{
    authum_timer_t timer;

    if (fn == 0)
    {
        return 0u;
    }
    if (!authum_timers_ready)
    {
        if (authum_null_eq(host) ||
            !authum_array_init(&authum_timers, authum_host_sandbox(host), sizeof(authum_timer_t)) ||
            !authum_host_subscribe(host, AUTHUM_EVENT_SERVER_FRAME, authum_timer_on_frame,
                                   authum_null))
        {
            return 0u;
        }
        authum_timers_ready = authum_bool_true;
    }
    timer.id = authum_timers_next_id++;
    if (authum_timers_next_id == 0u)
    {
        authum_timers_next_id = 1u;
    }
    timer.due = authum_timers_clock + authum_timer_us(delay);
    timer.interval = authum_timer_us(interval);
    timer.fn = fn;
    timer.user = user;
    timer.dead = authum_bool_false;
    return authum_null_eq(authum_array_push(&authum_timers, &timer)) ? 0u : timer.id;
}

authum_bool_t
authum_timer_remove(authum_u32_t id)
{
    authum_usize_t i;

    for (i = 0u; i < authum_array_size(&authum_timers); ++i)
    {
        authum_timer_t *timer = (authum_timer_t *)authum_array_at(&authum_timers, i);

        if (timer->id == id && !timer->dead)
        {
            timer->dead = authum_bool_true;
            if (!authum_timers_running)
            {
                authum_timer_sweep();
            }
            return authum_bool_true;
        }
    }
    return authum_bool_false;
}

void
authum_timer_clear(void)
{
    authum_usize_t i;

    for (i = 0u; i < authum_array_size(&authum_timers); ++i)
    {
        ((authum_timer_t *)authum_array_at(&authum_timers, i))->dead = authum_bool_true;
    }
    if (!authum_timers_running)
    {
        authum_timer_sweep();
    }
}

authum_usize_t
authum_timer_count(void)
{
    authum_usize_t i;
    authum_usize_t count = 0u;

    for (i = 0u; i < authum_array_size(&authum_timers); ++i)
    {
        count += ((authum_timer_t *)authum_array_at(&authum_timers, i))->dead ? 0u : 1u;
    }
    return count;
}

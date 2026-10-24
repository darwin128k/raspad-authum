/**
 * @file timers.c
 * @brief authum_timer_*: one-shot, repeating, removal from a callback, adding
 *        from a callback, long frames, clear — driven by authum.server.frame.
 */

#include <check.h>

#include <authum/client.h>
#include <authum/host/events.h>
#include <authum/host/sandbox.h>

#include <string.h>

int g_owner;
authum_host_t g_host;
authum_sandbox_t g_sandbox;
authum_events_t g_events;

int g_once;
int g_every;
int g_self_removing;
int g_spawned;
authum_u32_t g_self_id;

void
test_frame(authum_float_t seconds)
{
    authum_event_server_t ev;

    memset(&ev, 0, sizeof ev);
    ev.size = sizeof ev;
    ev.frame_time = seconds;
    (void)authum_events_host_publish(AUTHUM_EVENT_SERVER_FRAME, &ev);
}

/** @p seconds of 10 ms frames. */
void
test_run(authum_float_t seconds)
{
    int frames = (int)(seconds * 100.0f + 0.5f);
    int i;

    for (i = 0; i < frames; ++i)
    {
        test_frame(0.01f);
    }
}

void
test_on_once(authum_ptr user)
{
    authum_unused(user);
    ++g_once;
}

void
test_on_every(authum_ptr user)
{
    ++*(int *)user;
}

void
test_on_self_removing(authum_ptr user)
{
    authum_unused(user);
    ++g_self_removing;
    if (g_self_removing == 2)
    {
        CHECK(authum_timer_remove(g_self_id));
    }
}

void
test_on_spawner(authum_ptr user)
{
    authum_unused(user);
    /* Adding from inside a callback: the array may move under the loop. */
    CHECK(authum_timer_add(&g_host, 0.5f, 0.0f, test_on_every, &g_spawned) != 0u);
}

int
main(void)
{
    authum_u32_t every;

    g_sandbox.ops = authum_sandbox_host_ops();
    g_sandbox.root = authum_sandbox_host_root_create();
    g_events.ops = authum_events_host_ops();
    g_events.owner = &g_owner;
    memset(&g_host, 0, sizeof g_host);
    g_host.size = sizeof g_host;
    g_host.sandbox = &g_sandbox;
    g_host.events = &g_events;

    CHECK(authum_timer_add(&g_host, 1.0f, 0.0f, 0, authum_null) == 0u); /* no callback */

    CHECK(authum_timer_add(&g_host, 1.0f, 0.0f, test_on_once, authum_null) != 0u);
    every = authum_timer_add(&g_host, 0.5f, 0.5f, test_on_every, &g_every);
    CHECK(every != 0u);
    g_self_id = authum_timer_add(&g_host, 0.2f, 0.2f, test_on_self_removing, authum_null);
    CHECK(authum_timer_count() == 3u);

    test_run(0.95f);
    CHECK(g_once == 0);
    CHECK(g_every == 1);         /* at 0.5 */
    CHECK(g_self_removing == 2); /* at 0.2, 0.4 — then it removed itself */
    CHECK(authum_timer_count() == 2u);

    test_run(0.1f); /* 1.05 */
    CHECK(g_once == 1);
    CHECK(g_every == 2);
    CHECK(authum_timer_count() == 1u); /* the one-shot is gone */

    /* A 3-second frame (a hitch): one call, not a burst of catch-ups. */
    test_frame(3.0f);
    CHECK(g_every == 3);
    test_run(0.5f);
    CHECK(g_every == 4);

    CHECK(authum_timer_remove(every));
    CHECK(!authum_timer_remove(every));
    test_run(2.0f);
    CHECK(g_every == 4);
    CHECK(authum_timer_count() == 0u);

    CHECK(authum_timer_add(&g_host, 0.1f, 0.0f, test_on_spawner, authum_null) != 0u);
    test_run(0.15f);
    CHECK(authum_timer_count() == 1u); /* the spawned one waits */
    test_run(0.5f);
    CHECK(g_spawned == 1);

    authum_timer_add(&g_host, 1.0f, 1.0f, test_on_once, authum_null);
    authum_timer_add(&g_host, 2.0f, 0.0f, test_on_once, authum_null);
    authum_timer_clear();
    CHECK(authum_timer_count() == 0u);
    test_run(3.0f);
    CHECK(g_once == 1);

    authum_events_host_drop_owner(&g_owner);
    authum_sandbox_host_root_destroy(g_sandbox.root);
    return authum_check_report("timers");
}

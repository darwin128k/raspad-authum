/**
 * @file timer.cpp
 * @brief authum_timer_*: one-shot, repeating, removal from a callback, adding
 *        from a callback, long frames, clear — driven by authum.server.frame.
 */

#include <gtest/gtest.h>

#include <authum/client.h>
#include <authum/host/events.h>
#include <authum/host/sandbox.h>

#include <string.h>
#include <stdlib.h>

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
        EXPECT_TRUE(authum_timer_remove(g_self_id));
    }
}

void
test_on_spawner(authum_ptr user)
{
    authum_unused(user);
    /* Adding from inside a callback: the array may move under the loop. */
    EXPECT_NE(authum_timer_add(&g_host, 0.5f, 0.0f, test_on_every, &g_spawned), 0u);
}

void
timer_setup(void)
{
    g_once = g_every = g_self_removing = g_spawned = 0;
    g_sandbox.ops = authum_sandbox_host_ops();
    g_sandbox.root = authum_sandbox_host_root_create();
    authum_events_set_ops(&g_events, authum_events_host_ops());
    authum_events_set_owner(&g_events, &g_owner);
    memset(&g_host, 0, sizeof g_host);
    g_host.sandbox = &g_sandbox;
    g_host.events = &g_events;
}

void
timer_teardown(void)
{
    authum_timer_clear();
    authum_events_host_drop_owner(&g_owner);
    authum_sandbox_host_root_destroy(g_sandbox.root);
}

void
timer_rejects_missing_callback(void)
{
    EXPECT_EQ(authum_timer_add(&g_host, 1.0f, 0.0f, authum_null, authum_null), 0u);
}

void
timer_fires_once_after_delay(void)
{
    ASSERT_NE(authum_timer_add(&g_host, 1.0f, 0.0f, test_on_once, authum_null), 0u);
    test_run(0.95f);
    EXPECT_EQ(g_once, 0);
    test_run(0.1f);
    EXPECT_EQ(g_once, 1);
    EXPECT_EQ(authum_timer_get_count(), 0u);
    test_run(2.0f);
    EXPECT_EQ(g_once, 1);
}

void
timer_repeats_and_skips_catch_up_burst(void)
{
    ASSERT_NE(authum_timer_add(&g_host, 0.5f, 0.5f, test_on_every, &g_every), 0u);
    test_run(1.05f);
    EXPECT_EQ(g_every, 2);
    test_frame(3.0f);
    EXPECT_EQ(g_every, 3);
    test_run(0.5f);
    EXPECT_EQ(g_every, 4);
}

void
timer_callback_can_remove_itself(void)
{
    g_self_id = authum_timer_add(&g_host, 0.2f, 0.2f, test_on_self_removing, authum_null);
    ASSERT_NE(g_self_id, 0u);
    test_run(0.95f);
    EXPECT_EQ(g_self_removing, 2);
    EXPECT_EQ(authum_timer_get_count(), 0u);
}

void
timer_removed_callback_never_runs_again(void)
{
    auto id = authum_timer_add(&g_host, 0.5f, 0.5f, test_on_every, &g_every);
    ASSERT_NE(id, 0u);
    EXPECT_TRUE(authum_timer_remove(id));
    EXPECT_FALSE(authum_timer_remove(id));
    test_run(2.0f);
    EXPECT_EQ(g_every, 0);
    EXPECT_EQ(authum_timer_get_count(), 0u);
}

void
timer_callback_can_add_another_timer(void)
{
    ASSERT_NE(authum_timer_add(&g_host, 0.1f, 0.0f, test_on_spawner, authum_null), 0u);
    test_run(0.15f);
    EXPECT_EQ(authum_timer_get_count(), 1u);
    EXPECT_EQ(g_spawned, 0);
    test_run(0.5f);
    EXPECT_EQ(g_spawned, 1);
}

void
timer_clear_cancels_every_callback(void)
{
    ASSERT_NE(authum_timer_add(&g_host, 1.0f, 1.0f, test_on_once, authum_null), 0u);
    ASSERT_NE(authum_timer_add(&g_host, 2.0f, 0.0f, test_on_once, authum_null), 0u);
    authum_timer_clear();
    EXPECT_EQ(authum_timer_get_count(), 0u);
    test_run(3.0f);
    EXPECT_EQ(g_once, 0);
}

// A fresh child process reproduces a freshly loaded client image for each case.

TEST(authum_timer, rejects_missing_callback)
{
    ASSERT_EXIT(
        {
            timer_setup();
            timer_rejects_missing_callback();
            timer_teardown();
            exit(::testing::Test::HasFailure() ? 1 : 0);
        },
        ::testing::ExitedWithCode(0), "");
}

TEST(authum_timer, fires_once_after_delay)
{
    ASSERT_EXIT(
        {
            timer_setup();
            timer_fires_once_after_delay();
            timer_teardown();
            exit(::testing::Test::HasFailure() ? 1 : 0);
        },
        ::testing::ExitedWithCode(0), "");
}

TEST(authum_timer, repeats_and_skips_catch_up_burst)
{
    ASSERT_EXIT(
        {
            timer_setup();
            timer_repeats_and_skips_catch_up_burst();
            timer_teardown();
            exit(::testing::Test::HasFailure() ? 1 : 0);
        },
        ::testing::ExitedWithCode(0), "");
}

TEST(authum_timer, callback_can_remove_itself)
{
    ASSERT_EXIT(
        {
            timer_setup();
            timer_callback_can_remove_itself();
            timer_teardown();
            exit(::testing::Test::HasFailure() ? 1 : 0);
        },
        ::testing::ExitedWithCode(0), "");
}

TEST(authum_timer, removed_callback_never_runs_again)
{
    ASSERT_EXIT(
        {
            timer_setup();
            timer_removed_callback_never_runs_again();
            timer_teardown();
            exit(::testing::Test::HasFailure() ? 1 : 0);
        },
        ::testing::ExitedWithCode(0), "");
}

TEST(authum_timer, callback_can_add_another_timer)
{
    ASSERT_EXIT(
        {
            timer_setup();
            timer_callback_can_add_another_timer();
            timer_teardown();
            exit(::testing::Test::HasFailure() ? 1 : 0);
        },
        ::testing::ExitedWithCode(0), "");
}

TEST(authum_timer, clear_cancels_every_callback)
{
    ASSERT_EXIT(
        {
            timer_setup();
            timer_clear_cancels_every_callback();
            timer_teardown();
            exit(::testing::Test::HasFailure() ? 1 : 0);
        },
        ::testing::ExitedWithCode(0), "");
}

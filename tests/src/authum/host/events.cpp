/**
 * @file events.c
 * @brief Event bus: forwards, queries, stop, re-entrancy, owner cleanup.
 */

#include <gtest/gtest.h>
#include <cstddef>

#include <authum/event/client.h>
#include <authum/events.h>
#include <authum/host/events.h>
#include <authum/null.h>
#include <authum/unused.h>

#include <string.h>

authum_char_t g_trace[64];

void
test_trace(const authum_char_t *tag)
{
    strncat(g_trace, tag, sizeof g_trace - strlen(g_trace) - 1u);
}

authum_event_result_t
test_on_a(const authum_char_t *name, authum_ptr payload, authum_ptr user)
{
    authum_unused(name);
    authum_unused(payload);
    authum_unused(user);
    test_trace("a");
    return authum_event_result_continue;
}

authum_event_result_t
test_on_b(const authum_char_t *name, authum_ptr payload, authum_ptr user)
{
    authum_unused(name);
    authum_unused(payload);
    authum_unused(user);
    test_trace("b");
    return authum_event_result_handled;
}

authum_event_result_t
test_on_stop(const authum_char_t *name, authum_ptr payload, authum_ptr user)
{
    authum_unused(name);
    authum_unused(payload);
    authum_unused(user);
    test_trace("s");
    return authum_event_result_stop;
}

authum_events_t
test_events_for(authum_ptr owner)
{
    authum_events_t ev;

    authum_events_set_ops(&ev, authum_events_host_ops());
    authum_events_set_owner(&ev, owner);
    authum_events_set_prefix(&ev, "test");
    return ev;
}

TEST(authum_events, forward_order_and_results)
{
    int mod_a;
    int mod_b;
    authum_events_t a = test_events_for(&mod_a);
    authum_events_t b = test_events_for(&mod_b);

    g_trace[0] = '\0';
    EXPECT_TRUE(authum_events_subscribe(&a, "test.ping", test_on_a, authum_null));
    EXPECT_TRUE(authum_events_subscribe(&a, "test.ping", test_on_a, authum_null));
    EXPECT_TRUE(authum_events_subscribe(&b, "test.ping", test_on_b, authum_null));
    EXPECT_TRUE(authum_events_host_count() == 2u);

    EXPECT_TRUE(authum_events_forward(&a, "test.ping", authum_null) == authum_event_result_handled);
    EXPECT_STREQ(g_trace, "ab");
    EXPECT_TRUE(authum_events_forward(&a, "test.none", authum_null) ==
                authum_event_result_continue);

    EXPECT_TRUE(authum_events_subscribe(&a, "test.stop", test_on_stop, authum_null));
    EXPECT_TRUE(authum_events_subscribe(&b, "test.stop", test_on_b, authum_null));
    g_trace[0] = '\0';
    EXPECT_TRUE(authum_events_host_publish("test.stop", authum_null) == authum_event_result_stop);
    EXPECT_STREQ(g_trace, "s");

    authum_events_host_drop_owner(&mod_a);
    EXPECT_TRUE(authum_events_host_count() == 2u);
    g_trace[0] = '\0';
    authum_events_forward(&b, "test.ping", authum_null);
    EXPECT_STREQ(g_trace, "b");

    EXPECT_TRUE(authum_events_unsubscribe(&b, "test.ping", test_on_b, authum_null));
    EXPECT_TRUE(!authum_events_unsubscribe(&b, "test.ping", test_on_b, authum_null));
    authum_events_host_clear();
    EXPECT_TRUE(authum_events_host_count() == 0u);
}

struct test_query
{
    authum_usize_t size;
    authum_i32_t in_index;
    authum_bool_t out_found;
    authum_i32_t out_value;
};

authum_usize_t
test_query_get_size(const struct test_query *self)
{
    return self->size;
}

void
test_query_set_size(struct test_query *self, authum_usize_t value)
{
    self->size = value;
}

authum_i32_t
test_query_get_in_index(const struct test_query *self)
{
    return self->in_index;
}

void
test_query_set_in_index(struct test_query *self, authum_i32_t value)
{
    self->in_index = value;
}

authum_bool_t
test_query_get_out_found(const struct test_query *self)
{
    return self->out_found;
}

void
test_query_set_out_found(struct test_query *self, authum_bool_t value)
{
    self->out_found = value;
}

authum_i32_t
test_query_get_out_value(const struct test_query *self)
{
    return self->out_value;
}

void
test_query_set_out_value(struct test_query *self, authum_i32_t value)
{
    self->out_value = value;
}

authum_event_result_t
test_on_query(const authum_char_t *name, authum_ptr payload, authum_ptr user)
{
    struct test_query *q = static_cast<struct test_query *>(payload);

    authum_unused(name);
    authum_unused(user);
    if (test_query_get_size(q) < sizeof *q || test_query_get_in_index(q) != 3)
    {
        return authum_event_result_continue;
    }
    test_query_set_out_found(q, authum_bool_true);
    test_query_set_out_value(q, 333);
    return authum_event_result_handled;
}

TEST(authum_events, query)
{
    int provider;
    int consumer;
    authum_events_t p = test_events_for(&provider);
    authum_events_t c = test_events_for(&consumer);
    struct test_query q;

    EXPECT_TRUE(authum_events_subscribe(&p, "test.query", test_on_query, authum_null));

    memset(&q, 0, sizeof q);
    test_query_set_size(&q, sizeof q);
    test_query_set_in_index(&q, 3);
    EXPECT_TRUE(authum_events_query(&c, "test.query", &q) == authum_event_result_handled);
    EXPECT_TRUE(test_query_get_out_found(&q) && test_query_get_out_value(&q) == 333);

    memset(&q, 0, sizeof q);
    q.size = sizeof q;
    test_query_set_in_index(&q, 4);
    EXPECT_TRUE(authum_events_query(&c, "test.query", &q) == authum_event_result_continue);
    EXPECT_TRUE(!test_query_get_out_found(&q));

    authum_events_host_drop_owner(&provider);
    memset(&q, 0, sizeof q);
    q.size = sizeof q;
    q.in_index = 3;
    EXPECT_TRUE(authum_events_query(&c, "test.query", &q) == authum_event_result_continue);
    authum_events_host_clear();
}

authum_events_t g_self_removing;

authum_event_result_t
test_on_self_remove(const authum_char_t *name, authum_ptr payload, authum_ptr user)
{
    authum_unused(payload);
    authum_unused(user);
    test_trace("r");
    authum_events_unsubscribe(&g_self_removing, name, test_on_self_remove, authum_null);
    authum_events_forward(&g_self_removing, "test.nested", authum_null);
    return authum_event_result_continue;
}

TEST(authum_events, reentrancy)
{
    int owner;

    g_self_removing = test_events_for(&owner);
    EXPECT_TRUE(
        authum_events_subscribe(&g_self_removing, "test.re", test_on_self_remove, authum_null));
    EXPECT_TRUE(authum_events_subscribe(&g_self_removing, "test.re", test_on_a, authum_null));
    EXPECT_TRUE(authum_events_subscribe(&g_self_removing, "test.nested", test_on_b, authum_null));
    g_trace[0] = '\0';
    authum_events_host_publish("test.re", authum_null);
    EXPECT_STREQ(g_trace, "rba");
    g_trace[0] = '\0';
    authum_events_host_publish("test.re", authum_null);
    EXPECT_STREQ(g_trace, "a");
    EXPECT_TRUE(authum_events_host_count() == 2u);
    authum_events_host_clear();
}

authum_char_t *
test_client_get_name_mut(authum_event_client_t *self)
{
    return self->name;
}

authum_event_result_t
test_on_connect(const authum_char_t *name, authum_ptr payload, authum_ptr user)
{
    authum_event_client_t *ev = static_cast<authum_event_client_t *>(payload);

    authum_unused(name);
    authum_unused(user);
    strcpy(test_client_get_name_mut(ev), "seen");
    return authum_event_result_continue;
}

TEST(authum_events, a_client_event_is_a_message)
{
    int owner;
    authum_events_t ev = test_events_for(&owner);
    authum_event_client_t client;

    /* An event says what happened. Refusing a client is a hook
     * (::authum_hook_point_client_connect), not a field of the event. */
    EXPECT_TRUE(
        authum_events_subscribe(&ev, AUTHUM_EVENT_CLIENT_CONNECT, test_on_connect, authum_null));
    memset(&client, 0, sizeof client);
    strcpy(test_client_get_name_mut(&client), "bad");
    authum_events_host_publish(AUTHUM_EVENT_CLIENT_CONNECT, &client);
    EXPECT_STREQ(test_client_get_name_mut(&client), "seen");
    authum_events_host_clear();
}

TEST(authum_events, invalid_args)
{
    authum_events_t ev = test_events_for(authum_null);
    authum_char_t long_name[80];

    memset(long_name, 'x', sizeof long_name - 1u);
    long_name[sizeof long_name - 1u] = '\0';
    EXPECT_TRUE(!authum_events_subscribe(authum_null, "x", test_on_a, authum_null));
    EXPECT_TRUE(!authum_events_subscribe(&ev, "", test_on_a, authum_null));
    EXPECT_TRUE(!authum_events_subscribe(&ev, long_name, test_on_a, authum_null));
    EXPECT_TRUE(!authum_events_subscribe(&ev, "x", 0, authum_null));
    EXPECT_TRUE(authum_events_forward(authum_null, "x", authum_null) ==
                authum_event_result_continue);
}

TEST(authum_events, numeric_api_shares_registry_and_deduplicates_string_subscriptions)
{
    int owner;
    auto events = test_events_for(&owner);
    auto id = authum_events_get_id(&events, "test.numeric");
    ASSERT_NE(id, 0u);
    EXPECT_EQ(authum_events_get_id(&events, "test.numeric"), id);
    EXPECT_TRUE(authum_events_subscribe(&events, "test.numeric", test_on_b, authum_null));
    EXPECT_TRUE(authum_events_subscribe_id(&events, id, test_on_b, authum_null));
    EXPECT_EQ(authum_events_host_count(), 1u);
    g_trace[0] = '\0';
    EXPECT_EQ(authum_events_forward(&events, "test.numeric", authum_null), authum_event_result_handled);
    EXPECT_STREQ(g_trace, "b");
    EXPECT_TRUE(authum_events_unsubscribe_id(&events, id, test_on_b, authum_null));
    EXPECT_FALSE(authum_events_unsubscribe(&events, "test.numeric", test_on_b, authum_null));
    authum_events_host_clear();
    EXPECT_EQ(authum_events_get_id(&events, "test.numeric"), id);
    EXPECT_FALSE(authum_events_subscribe_id(&events, 0, test_on_a, authum_null));
    EXPECT_EQ(authum_events_forward(&events, "other.name", authum_null), authum_event_result_continue);
    EXPECT_EQ(authum_events_get_id(&events, ""), 0u);
    EXPECT_EQ(authum_events_get_id(authum_null, "test.numeric"), 0u);
}

TEST(authum_events, host_cache_converts_id_values_without_aliasing_module_storage)
{
    int owner;
    auto events = test_events_for(&owner);
    authum_event_id_t cached = 0;
    EXPECT_EQ(authum_events_host_get_cached_id(&cached, authum_null), 0u);
    EXPECT_EQ(cached, 0u);
    const auto id = authum_events_host_get_cached_id(&cached, "test.host.cached");
    ASSERT_NE(id, 0u);
    EXPECT_EQ(cached, id);
    EXPECT_EQ(authum_events_get_id(&events, "test.host.cached"), id);
    EXPECT_EQ(authum_events_host_get_cached_id(&cached, "test.host.cached"), id);
    EXPECT_TRUE(authum_events_subscribe_id(&events, cached, test_on_b, authum_null));
    EXPECT_EQ(authum_events_host_publish_id(cached, authum_null), authum_event_result_handled);
    authum_events_host_clear();
    EXPECT_EQ(authum_events_host_get_cached_id(&cached, "test.host.cached"), id);
}

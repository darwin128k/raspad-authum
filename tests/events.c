/**
 * @file events.c
 * @brief Event bus: forwards, queries, stop, re-entrancy, owner cleanup.
 */

#include <check.h>

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

    ev.ops = authum_events_host_ops();
    ev.owner = owner;
    return ev;
}

void
test_forward_order_and_results(void)
{
    int mod_a;
    int mod_b;
    authum_events_t a = test_events_for(&mod_a);
    authum_events_t b = test_events_for(&mod_b);

    g_trace[0] = '\0';
    CHECK(authum_events_subscribe(&a, "test.ping", test_on_a, authum_null));
    CHECK(authum_events_subscribe(&a, "test.ping", test_on_a, authum_null));
    CHECK(authum_events_subscribe(&b, "test.ping", test_on_b, authum_null));
    CHECK(authum_events_host_count() == 2u);

    CHECK(authum_events_publish(&a, "test.ping", authum_null) == authum_event_result_handled);
    CHECK_STR(g_trace, "ab");
    CHECK(authum_events_publish(&a, "test.none", authum_null) == authum_event_result_continue);

    CHECK(authum_events_subscribe(&a, "test.stop", test_on_stop, authum_null));
    CHECK(authum_events_subscribe(&b, "test.stop", test_on_b, authum_null));
    g_trace[0] = '\0';
    CHECK(authum_events_host_publish("test.stop", authum_null) == authum_event_result_stop);
    CHECK_STR(g_trace, "s");

    authum_events_host_drop_owner(&mod_a);
    CHECK(authum_events_host_count() == 2u);
    g_trace[0] = '\0';
    authum_events_publish(&b, "test.ping", authum_null);
    CHECK_STR(g_trace, "b");

    CHECK(authum_events_unsubscribe(&b, "test.ping", test_on_b, authum_null));
    CHECK(!authum_events_unsubscribe(&b, "test.ping", test_on_b, authum_null));
    authum_events_host_clear();
    CHECK(authum_events_host_count() == 0u);
}

struct test_query
{
    authum_usize_t size;
    authum_i32_t in_index;
    authum_bool_t out_found;
    authum_i32_t out_value;
};

authum_event_result_t
test_on_query(const authum_char_t *name, authum_ptr payload, authum_ptr user)
{
    struct test_query *q = (struct test_query *)payload;

    authum_unused(name);
    authum_unused(user);
    if (q->size < sizeof *q || q->in_index != 3) {
        return authum_event_result_continue;
    }
    q->out_found = authum_bool_true;
    q->out_value = 333;
    return authum_event_result_handled;
}

void
test_query(void)
{
    int provider;
    int consumer;
    authum_events_t p = test_events_for(&provider);
    authum_events_t c = test_events_for(&consumer);
    struct test_query q;

    CHECK(authum_events_subscribe(&p, "test.query", test_on_query, authum_null));

    memset(&q, 0, sizeof q);
    q.size = sizeof q;
    q.in_index = 3;
    CHECK(authum_events_publish(&c, "test.query", &q) == authum_event_result_handled);
    CHECK(q.out_found && q.out_value == 333);

    memset(&q, 0, sizeof q);
    q.size = sizeof q;
    q.in_index = 4;
    CHECK(authum_events_publish(&c, "test.query", &q) == authum_event_result_continue);
    CHECK(!q.out_found);

    authum_events_host_drop_owner(&provider);
    memset(&q, 0, sizeof q);
    q.size = sizeof q;
    q.in_index = 3;
    CHECK(authum_events_publish(&c, "test.query", &q) == authum_event_result_continue);
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
    authum_events_publish(&g_self_removing, "test.nested", authum_null);
    return authum_event_result_continue;
}

void
test_reentrancy(void)
{
    int owner;

    g_self_removing = test_events_for(&owner);
    CHECK(authum_events_subscribe(&g_self_removing, "test.re", test_on_self_remove, authum_null));
    CHECK(authum_events_subscribe(&g_self_removing, "test.re", test_on_a, authum_null));
    CHECK(authum_events_subscribe(&g_self_removing, "test.nested", test_on_b, authum_null));
    g_trace[0] = '\0';
    authum_events_host_publish("test.re", authum_null);
    CHECK_STR(g_trace, "rba");
    g_trace[0] = '\0';
    authum_events_host_publish("test.re", authum_null);
    CHECK_STR(g_trace, "a");
    CHECK(authum_events_host_count() == 2u);
    authum_events_host_clear();
}

authum_event_result_t
test_on_connect_refuse(const authum_char_t *name, authum_ptr payload, authum_ptr user)
{
    authum_event_client_t *ev = (authum_event_client_t *)payload;

    authum_unused(name);
    authum_unused(user);
    if (ev->size < sizeof *ev || strcmp(ev->name, "bad") != 0) {
        return authum_event_result_continue;
    }
    ev->allow = authum_bool_false;
    strcpy(ev->reject_reason, "no");
    return authum_event_result_stop;
}

void
test_client_connect_refusal(void)
{
    int owner;
    authum_events_t ev = test_events_for(&owner);
    authum_event_client_t client;

    CHECK(authum_events_subscribe(&ev, AUTHUM_EVENT_CLIENT_CONNECT, test_on_connect_refuse,
                                  authum_null));
    memset(&client, 0, sizeof client);
    client.size = sizeof client;
    client.allow = authum_bool_true;
    strcpy(client.name, "bad");
    authum_events_host_publish(AUTHUM_EVENT_CLIENT_CONNECT, &client);
    CHECK(!client.allow);
    CHECK_STR(client.reject_reason, "no");
    authum_events_host_clear();
}

void
test_invalid_args(void)
{
    authum_events_t ev = test_events_for(authum_null);
    authum_char_t long_name[80];

    memset(long_name, 'x', sizeof long_name - 1u);
    long_name[sizeof long_name - 1u] = '\0';
    CHECK(!authum_events_subscribe(authum_null, "x", test_on_a, authum_null));
    CHECK(!authum_events_subscribe(&ev, "", test_on_a, authum_null));
    CHECK(!authum_events_subscribe(&ev, long_name, test_on_a, authum_null));
    CHECK(!authum_events_subscribe(&ev, "x", 0, authum_null));
    CHECK(authum_events_publish(authum_null, "x", authum_null) == authum_event_result_continue);
}

int
main(void)
{
    test_forward_order_and_results();
    test_query();
    test_reentrancy();
    test_client_connect_refusal();
    test_invalid_args();
    return authum_check_report("events");
}

#include <authum/client/ref.h>
#include <authum/host/events.h>
#include <authum/event/client.h>
#include <authum/event/server.h>
#include <gtest/gtest.h>

static authum_i32_t current_user = 100;

authum_i32_t
userid(authum_i32_t index)
{
    return index == 1 ? current_user : 0;
}

void
publish_client(const authum_host_t *host, const char *event)
{
    authum_event_client_t ev = {0};
    ev.index = 1;
    ev.userid = current_user;
    authum_host_forward(host, event, &ev);
}

static authum_clients_t clients, other;
static authum_host_t host;
static authum_engine_t engine;
static authum_events_t events;
static authum_client_ref_t ref;

namespace tests
{
class authum_client_ref : public ::testing::Test
{
  protected:
    void
    SetUp() override
    {
        current_user = 100;
        clients = {};
        other = {};
        host = {};
        engine = {};
        events = {authum_events_host_ops(), &host, "authum"};
        host.events = &events;
        host.engine = &engine;
        engine.client_userid = userid;
        ASSERT_TRUE(authum_clients_init(&clients, &host, 32));
        ASSERT_TRUE(authum_clients_capture(&clients, 1, &ref));
    }

    void
    TearDown() override
    {
        authum_clients_destroy(&other);
        authum_clients_destroy(&clients);
        authum_events_host_drop_owner(&host);
    }
};

TEST_F(authum_client_ref, recognizes_connection_existing_at_module_load)
{
    EXPECT_TRUE(authum_client_ref_is_current(&clients, &ref));
}

TEST_F(authum_client_ref, another_tracker_cannot_validate_reference)
{
    ASSERT_TRUE(authum_clients_init(&other, &host, 32));
    EXPECT_FALSE(authum_client_ref_is_current(&other, &ref));
}

TEST_F(authum_client_ref, disconnect_invalidates_reference)
{
    publish_client(&host, AUTHUM_EVENT_CLIENT_DISCONNECT);
    EXPECT_FALSE(authum_client_ref_is_current(&clients, &ref));
}

TEST_F(authum_client_ref, reconnect_with_same_userid_changes_generation)
{
    publish_client(&host, AUTHUM_EVENT_CLIENT_DISCONNECT);
    publish_client(&host, AUTHUM_EVENT_CLIENT_CONNECT);
    publish_client(&host, AUTHUM_EVENT_CLIENT_PUT_IN_SERVER);
    EXPECT_FALSE(authum_client_ref_is_current(&clients, &ref));
    ASSERT_TRUE(authum_clients_capture(&clients, 1, &ref));
    EXPECT_TRUE(authum_client_ref_is_current(&clients, &ref));
}

TEST_F(authum_client_ref, changed_userid_invalidates_reference)
{
    current_user = 101;
    EXPECT_FALSE(authum_client_ref_is_current(&clients, &ref));
}

TEST_F(authum_client_ref, map_deactivation_invalidates_reference)
{
    authum_host_forward(&host, AUTHUM_EVENT_SERVER_DEACTIVATE, authum_null);
    EXPECT_FALSE(authum_client_ref_is_current(&clients, &ref));
}

TEST_F(authum_client_ref, rejects_out_of_range_slots)
{
    EXPECT_FALSE(authum_clients_capture(&clients, 0, &ref));
    EXPECT_FALSE(authum_clients_capture(&clients, 33, &ref));
}

TEST_F(authum_client_ref, tracker_reinitialization_changes_lifetime)
{
    authum_clients_destroy(&clients);
    ASSERT_TRUE(authum_clients_init(&clients, &host, 32));
    EXPECT_FALSE(authum_client_ref_is_current(&clients, &ref));
}

TEST_F(authum_client_ref, destroying_tracker_twice_is_safe)
{
    authum_clients_destroy(&clients);
    authum_clients_destroy(&clients);
}
} // namespace tests
